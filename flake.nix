{
  description = "A flake exposing the header-only PjPlots library";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.11";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = {
    nixpkgs,
    flake-utils,
    ...
  }:
    flake-utils.lib.eachSystem ["x86_64-linux" "aarch64-linux" "aarch64-darwin"] (system: let
      pkgs = import nixpkgs {
        inherit system;
      };
    in {
      packages.default = pkgs.stdenv.mkDerivation {
        pname = "PjPlots";
        version = "1.0.0";
        src = ./.;

        nativeBuildInputs = [
          pkgs.cmake
          pkgs.ninja
        ];

        dontBuild = true;

        cmakeFlags = [
          "-DCMAKE_BUILD_TYPE=Release"
          "-DPJPLOTS_ENABLE_TESTS=OFF"
        ];

        meta = with pkgs.lib; {
          description = "Header-only modern C++ plotting library";
          homepage = "https://github.com/PatJRobinson/pjplots";
          license = licenses.bsd3;
          platforms = platforms.linux;
        };
      };

      devShells.default = pkgs.mkShell {
        packages = [
          pkgs.cmake
          pkgs.ninja
        ];
      };
    });
}
