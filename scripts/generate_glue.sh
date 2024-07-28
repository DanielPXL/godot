GD_EXE=$(find bin -name "*.mono.exe" -print0 | xargs -r -0 ls -1 -t | head -1)
"${GD_EXE}" --headless --generate-mono-glue modules/mono/glue
python modules/mono/build_scripts/build_assemblies.py --godot-output-dir=./bin --push-nupkgs-local ../../C#/NuGet/GodotLocal