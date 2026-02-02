import os
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain,cmake_layout


class ExampleRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("boost/1.89.0")
        self.requires("gtest/1.17.0")

    def layout(self):
        cmake_layout(self)
    def generate(self):
        # 1. Path to the actual file (e.g., project/build/Release/compile_commands.json)
        # target_file = os.path.join(self.build_folder, "compile_commands.json") # build_folder is Release/Debug
        
        # Or if you want a fixed path based on your custom setup:
        target_file = os.path.join(self.recipe_folder, "build", "Release", "compile_commands.json")
        
        # 2. Path for the symlink: './build/compile_commands.json'
        link_dir = os.path.join(self.recipe_folder, "build")
        link_name = os.path.join(link_dir, "compile_commands.json")

        # 3. Ensure the './build' directory exists so we can put a link in it
        if not os.path.exists(link_dir):
            os.makedirs(link_dir)
            
        # Ensure the subfolder (Release/Debug) also exists so the link has a destination
        target_dir = os.path.dirname(target_file)
        if not os.path.exists(target_dir):
            os.makedirs(target_dir)

        # 4. Remove old link if it exists
        if os.path.lexists(link_name):
            os.remove(link_name)

        # 5. Create the symlink
        try:
            # We use a relative symlink so the project remains portable
            # This links './build/compile_commands.json' -> './Release/compile_commands.json'
            relative_target = os.path.relpath(target_file, link_dir)
            os.symlink(relative_target, link_name)
            self.output.info(f"Symlink created in build folder: {link_name} -> {relative_target}")
        except OSError as e:
            self.output.warning(f"Could not create symlink: {e}")