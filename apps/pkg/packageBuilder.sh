#!/bin/bash
set -e

PKG_DIR="$1"
KEY="$2"

if [ -z "$PKG_DIR" ] || [ -z "$KEY" ]; then
	echo "Usage: ./packageBuilder.sh <package-folder> <private-key>"
	exit 1
fi

if [ ! -f "$KEY" ]; then
	echo "[X] Private key not found!"
	exit 1
fi

NAME=$(grep "^exec=" "$PKG_DIR/.packageInfo" | cut -d'=' -f2 | xargs basename)

HOOK_PATH=$(grep "^installHook=" "$PKG_DIR/.packageInfo" | cut -d'=' -f2)

PACKAGE_INFO="$PKG_DIR/.packageInfo"

LIB_PATH=$(grep "^lib=" "$PACKAGE_INFO" | cut -d'=' -f2)

LIB_PATH="$PKG_DIR/$LIB_PATH"

if [ ! -f "$PACKAGE_INFO" ]; then
    echo "[X] .packageInfo missing!"
    exit 1
fi

EXEC_PATH=$(grep "^exec=" "$PACKAGE_INFO" | cut -d'=' -f2)

VERSION=$(grep "^version=" "$PACKAGE_INFO" | cut -d'=' -f2)

if [ -z "$EXEC_PATH" ]; then
    echo "[X] exec= missing in .packageInfo"
    exit 1
fi

if [ -f "$PKG_DIR.boxpkg" ]; then
    echo "[*] Removing old $PKG_DIR.boxpkg build..."
    rm "$PKG_DIR.boxpkg"
fi

if [ -f "$HOOK_PATH" ]; then
	echo "[*] Including hook... (installHook)"
fi

EXEC_PATH="${EXEC_PATH#/}"
FULL_EXEC="$PKG_DIR/$EXEC_PATH"

if [ ! -f "$FULL_EXEC" ]; then
    echo "[X] Executable not found inside package root: $EXEC_PATH"
    exit 1
fi

mkdir -p "$PKG_DIR/lib"
mkdir -p "$PKG_DIR/lib64"

if [ -d "$LIB_PATH" ]; then
    echo "[*] Using provided libraries..."
fi

if [ ! -d "$LIB_PATH" ]; then
    echo "[*] Finding libraries needed..."

    ldd "$FULL_EXEC" | while read -r line; do
        LIB_PATH=$(echo "$line" | awk '{print $3}')

        if [[ -z "$LIB_PATH" || "$LIB_PATH" == "not" ]]; then
            continue
        fi

        if [ -f "$LIB_PATH" ]; then
            if [[ "$LIB_PATH" == *"/lib64/"* ]]; then
                cp -L "$LIB_PATH" "$PKG_DIR/lib64/"
            else
                cp -L "$LIB_PATH" "$PKG_DIR/lib/"
            fi
        fi
    done
fi

ZIP_NAME="$PKG_DIR.zip"

trap 'rm -f "$ZIP_NAME" sig.bin' EXIT

echo "[*] Zipping package..."
cd "$PKG_DIR"
zip -r "../$ZIP_NAME" . > /dev/null
cd ..

echo "[*] Signing package..."
openssl dgst -sha256 -sign "$KEY" "$ZIP_NAME" > sig.bin

echo "[*] Encoding..."
ZIP_B64=$(base64 -w 0 "$ZIP_NAME")
SIG_B64=$(base64 -w 0 sig.bin)

echo "[*] Building .boxpkg file..."

read -p "Description: " desc

if [ -z "$VERSION" ]; then
    read -p "Version: " VERSION
fi

cat > "$PKG_DIR.boxpkg" <<EOF
name=$NAME
description=$desc
version=$VERSION
b64=$ZIP_B64
signature=$SIG_B64
EOF

rm "$ZIP_NAME" sig.bin

echo "[!] Finished building: $PKG_DIR.boxpkg"
