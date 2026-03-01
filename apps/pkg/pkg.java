import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.nio.file.DirectoryStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardOpenOption;
import java.security.KeyFactory;
import java.security.PublicKey;
import java.security.Signature;
import java.security.spec.X509EncodedKeySpec;
import java.util.ArrayList;
import java.util.Base64;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

public class pkg {
	// static Scanner sc = new Scanner(System.in);

	static boolean local = false;

	static boolean debug = false;

	public static void help() {
		System.out.println("Usage:\n\npkg unpack [package] -- Installs package from local file [package].boxpkg.");
		System.out.println("pkg return [package] -- Removes package.\n");
		System.out.println("pkg scalp [Options] --essential — Downloads and upgrades packages, as well as the repository. (--essential upgrades and replaces essential packages)");
		System.out.println("pkg help -- Shows this screen.\n");
		System.out.println("PLEASE NOTE: To install a local package, you must pass the --local flag.");
	}

	public static void addPackageToRegistry(String pkgPath, String version) {
		Path packageList = Paths.get("/etc/pkg/.package_list");
		try {
			if (!Files.exists(packageList)) {
				Files.createFile(packageList);
			}

		String entry = "\npackage {\n" +
						"	pkg=" + pkgPath + "\n" +
						"	ver=" + version + "\n" +
						"}\n";

		Files.write(packageList, entry.getBytes(), StandardOpenOption.APPEND);
		if (debug) {
			System.out.println("[*] Added package to /etc/pkg/.package_list");
		}
		} catch (IOException e) {
			System.out.println("[X] Failed to update .package_list: " + e.getMessage());
		}
	}

	public static void removePackageFromRegistry(String pkgPath) {
		Path packageList = Paths.get("/etc/pkg/.package_list");
		if (!Files.exists(packageList)) return;

		try {
			List<String> lines = Files.readAllLines(packageList);
			List<String> newLines = new ArrayList<>();

			boolean skip = false;
			boolean insidePackage = false;
			
			for (String line : lines) {
				String trimmed = line.trim();
				if (trimmed.startsWith("package {")) {
					insidePackage = true;
					skip = false;
				}
				if (insidePackage && trimmed.startsWith("pkg=") && trimmed.substring(4).equals(pkgPath)) {
					skip = true;
				}
				
				if (!skip) {
					newLines.add(line);
				}

				if (insidePackage && trimmed.equals("}")) {
					insidePackage = false;
					skip = false;
				}
			}

			Files.write(packageList, newLines);
			if (debug) {
				System.out.println("[*] Removed package from .package_list");
			}
			} catch(IOException e) {
				System.out.println("[X] Failed to update .package_list: " + e.getMessage());
			}
		}

	public static PublicKey loadPublicKey(Path path) throws Exception {
		String key = new String(Files.readAllBytes(path));
		
		key = key.replace("-----BEGIN PUBLIC KEY-----", "")
			.replace("-----END PUBLIC KEY-----", "")
			.replaceAll("\\s+", "");

		byte[] decoded = Base64.getDecoder().decode(key);

		X509EncodedKeySpec spec = new X509EncodedKeySpec(decoded);
		KeyFactory kf = KeyFactory.getInstance("RSA");

		return kf.generatePublic(spec);
	}

	public static boolean verifySignature(byte[] data, String b64Signature, PublicKey key) {
	    try {
	        byte[] sigBytes = Base64.getDecoder().decode(b64Signature);
	
	        Signature sig = Signature.getInstance("SHA256withRSA");
	        sig.initVerify(key);
	        sig.update(data);
	
	        return sig.verify(sigBytes);
	    } catch (Exception e) {
	        return false;
	    }
	}

	public static boolean verifyWithTrustStore(byte[] data, String signature) {
	    Path trustDir = Paths.get("/etc/pkg/trusted");
	
	    if (!Files.exists(trustDir)) {
	        System.out.println("[X] Trust store not found.");
	        return false;
	    }
	
	    try (DirectoryStream<Path> stream = Files.newDirectoryStream(trustDir, "*.pub")) {
	
	        for (Path keyPath : stream) {
	            PublicKey key = loadPublicKey(keyPath);
	
	            if (verifySignature(data, signature, key)) {
	                if (debug) {
	                    System.out.println("[*] Verified with key: " + keyPath.getFileName());
	                }
	                return true;
	            }
	        }
	
	    } catch (Exception e) {
	        System.out.println("[X] Verification error: " + e.getMessage());
	        return false;
	    }
	
	    return false;
	}

	public static void runHook(Path hookPath) {
		try {
			List<String> lines = Files.readAllLines(hookPath);

			String lang = null;
			String script = null;

			Boolean knownLanguage = false;

			for (String line : lines) {
				line = line.trim();
				if (line.startsWith("lang=")) {
					lang = line.substring(5).trim();
				}
				if (line.startsWith("script=(") && line.endsWith(")")) {
					script = line.substring(8, line.length() - 1);
				}
			}

			if (lang == null || script == null) {
				System.out.println("[X] Invalid hook format.");
				return;
			}

			if (lang.equalsIgnoreCase("shell")) {
				knownLanguage = true;

				ProcessBuilder pb = new ProcessBuilder("/bin/sh", "-c", script);
				pb.inheritIO();
				Process p = pb.start();
				p.waitFor();
			}

			if (!knownLanguage) {
				System.out.println("[X] Hook language unknown.");
				return;
			}
		} catch (Exception e) {
			System.out.println("[X] Hook exec failed: " + e.getMessage());
		}
	}

	public static void returnPkg(String pkg) {
		
	}

	public static void install(String pkg) {
		File tmpDir = new File("/tmp");

		if (!tmpDir.exists()) {
			tmpDir.mkdirs();
		}
	
		if (local) {
			System.out.println("Attempting to install local package \"" + pkg + "\"...");

			try {
				String extension = ".boxpkg";

				File file = new File(pkg);
				BufferedReader reader = new BufferedReader(new FileReader(file));

				if (!file.getName().endsWith(extension)) {
					System.out.println("[X] 1 Check Failed!\nExiting...\n\n");
					System.exit(0);
				}
				
				String line;
				String name = null;
				String description = null;
				String version = null;
				String b64zip = null;
				String signature = null;

				
				while ((line = reader.readLine()) != null) {
					line = line.trim();
					if (line.isEmpty() || !line.contains("=")) continue;

					String[] parts = line.split("=", 2);
					String key = parts[0].trim();
					String value = parts[1].trim();

					switch (key) {
						case "name":
							name = value;
							break;
						case "description":
							description = value;
							break;
						case "version":
							version = value;
							break;
						case "b64":
							b64zip = value;
							break;
						case "signature":
							signature = value;
							break;
					}
					
					if (debug) {
						System.out.println(line);
					}
				}

				if (name == null || description == null || b64zip == null || signature == null) {
					System.out.println("[X] \"" + pkg + "\" is corrupted! Contact the author of this file.");
					reader.close();
					System.exit(0);
				}

				reader.close();

				System.out.println("--Package info--\nName: " + name);
				System.out.println("Description: " + description);

				File outFile = new File(tmpDir, name + ".zip");

				File extractDir = new File(tmpDir, name);

				if (b64zip != null) {
					byte[] zipData = Base64.getDecoder().decode(b64zip);

					boolean verified = verifyWithTrustStore(zipData, signature);

					if (!verified) {
						System.out.println("[X] Could not verify signature! Cancelling..");
						System.exit(1);
					}

					System.out.println("[!] Package verified!");
					
					try (FileOutputStream fos = new FileOutputStream(outFile)) {
						fos.write(zipData);
						if (debug) { System.out.println("Stored " + name + ".zip successfully to tmp/ directory."); }
					} catch (IOException e) {
						System.out.println("Error Message: " + e.getMessage());
					} 
				} else {
					System.out.println("[X] \"" + pkg + "\" is corrupted! Contact the author of this file.");
					reader.close();
					System.exit(0);
				}

				// Unzip Archive

				if (!extractDir.exists()) {
					extractDir.mkdirs();
				}

				try (ZipInputStream zis = new ZipInputStream(new FileInputStream(outFile))) {
					ZipEntry entry;
					while ((entry = zis.getNextEntry()) != null) {
						File outFile2 = new File(extractDir, entry.getName());

						if (entry.isDirectory()) {
							outFile2.mkdirs();
						} else {
							outFile.getParentFile().mkdirs();
							try (FileOutputStream fos = new FileOutputStream(outFile2)) {
								byte[] buffer = new byte[4096];
								int len;
								while ((len = zis.read(buffer)) > 0) {
									fos.write(buffer, 0, len);
								}
							}
						}
						zis.closeEntry();
					}
				}
				if (debug) {
					System.out.println("Unpacked .zip to " + extractDir.getAbsolutePath());
				}

				File data = new File(extractDir, ".packageInfo");
				if (!data.exists()) {
					System.out.println("[X] Installation data not in package!");
					if (debug) {
						System.out.println("Directory: " + extractDir.getAbsolutePath());
						System.out.println("[X] FILE MISSING: .packageInfo");
					}

					System.exit(0);
				} else {
					System.out.println("[*] Installing...");
					try {

						File fileBuffer2 = new File(extractDir, ".packageInfo");
						BufferedReader reader2 = new BufferedReader(new FileReader(fileBuffer2));

						String exec = null;
						String extraHook = null;
						
						String pkgInfoData;
						
						while ((pkgInfoData = reader2.readLine()) != null) {
							pkgInfoData = pkgInfoData.trim();
							if (pkgInfoData.isEmpty() || !pkgInfoData.contains("=")) continue;

							String[] parts = pkgInfoData.split("=", 2);
							String key = parts[0].trim();
							String value = parts[1].trim();

							switch (key) {
								case "exec":
									exec = value;
									break;
								// lib= is only for the package builder
								case "installHook":
									extraHook = value;
									break;
							}
						}

					if ((exec != null && !exec.isEmpty())) {
							Path pkgInstalls = Paths.get("/etc/pkg", name);
							Files.createDirectories(pkgInstalls);

							try {
								new ProcessBuilder("cp", "-aL", extractDir.getAbsolutePath() + "/.", "/etc/pkg/" + name + "/")
									.inheritIO()
									.start()
									.waitFor();
							} catch (IOException | InterruptedException e) {
								System.out.println("[X] ERROR! "+ e.getMessage());
								System.exit(0);
							}
							
						} else {
							System.out.println("[X] Package data incomplete/corrupted! Cannot install package! Returning...");
							System.exit(0);
						}

						reader2.close();

						if (debug) {
							System.out.println("[*] Creating symlinks...");
						}

						Path pkgRoot = Paths.get("/etc/pkg", name);

						Path[] dirsToLink = new Path[] {
							pkgRoot.resolve("bin"),
							pkgRoot.resolve("lib"),
							pkgRoot.resolve("lib64")	
						};

						Path[] targets = new Path[] {
							Paths.get("/bin"),
							Paths.get("/lib"),
							Paths.get("/lib64")	
						};

						for (int i = 0; i < dirsToLink.length; i++) {
						    Path sourceDir = dirsToLink[i];
						    Path targetDir = targets[i];
						
						    if (!Files.exists(sourceDir)) continue; // skip missing dirs
						
						    try {
						        Files.list(sourceDir).forEach(child -> { // renamed from 'file' to 'child'
						            Path link = targetDir.resolve(child.getFileName());
						            try {
						                if (!Files.exists(link)) {
						                    Files.createSymbolicLink(link, child.toAbsolutePath());
						                    if (debug) {
						                        System.out.println("[*] Symlinked " + child + " -> " + link);
						                    }
						                } else if (debug) {
						                    System.out.println("[!] Skipped existing: " + link);
						                }
						            } catch (Exception e) {
						                System.out.println("[X] Failed symlink " + child + ": " + e.getMessage());
						            }
						        });
						    } catch (IOException e) {
						        System.out.println("[X] Error reading directory " + sourceDir + ": " + e.getMessage());
						    }
						}

						if (extraHook != null && !extraHook.isEmpty()) {
							Path hookPath = extractDir.toPath()
														.resolve(extraHook)
														.normalize();

							if (Files.exists(hookPath)) {
								if (debug) {
									System.out.println("[*] Running hook(s)...");
								}

								runHook(hookPath);
								} else {
									System.out.println("[X] Hook not found: " + hookPath);
								}
						   	}
						
						} catch (IOException e) {
							System.out.println("Error Message: " + e.getMessage());
							System.out.println("Failed to install package \"" + pkg + "\"");
							System.exit(0);
						}

						System.out.println("[!] Package Installed!");
						
						addPackageToRegistry(name, version);
				}
				
			} catch (IOException e) {
				System.out.println("Error Message: " + e.getMessage());
				System.out.println("Failed to install package \"" + pkg + "\"...");
				System.exit(0);
			}
			
		} else {
			System.out.println("Attempting to install global package \"" + pkg + "\"...");
			// System.out.flush();
		}
	}
	
	public static void main(String[] args) {
		String command = null;
		String packageName = null;

		if (args.length == 0) {
			help();
			return;
		}

		for (String arg : args) {
			if (arg.equals("--local")) {
				local = true;
			} else if (arg.equals("--debug")) {
				debug = true;
			} else if (command == null) {
				command = arg;
			} else if (packageName == null) {
				packageName = arg;
			}
		}

		if (debug) {
			System.out.println("Command: " + command + "\nPackage: " + packageName);
		}

		switch (command) {
			case "unpack":
				if (packageName == null) {
					help();
					return;
				}
				install(packageName);
				break;
			case "help" :
				help();
				return;
			case "return" :
				if (packageName == null) {
					help();
					return;
				}
				returnPkg(packageName);
				break;
				
			default:
				help();
				break;
		}
	}	
}