{
  description = "Dynamic memory allocator";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
  let
    system = "x86_64-linux";
    pkgs = nixpkgs.legacyPackages.${system};
  in {
    packages.${system} = {
      default = pkgs.stdenv.mkDerivation {
        name = "Mem-Alloc";
        version = "0.0.0";
        src = ./.;

        nativeBuildInputs = with pkgs; [ gcc gnumake pkg-config ];
        # buildInputs = with pkgs; [ ];

        buildPhase = ''
          make 
        '';

        installPhase = ''
          mkdir -p $out/lib $out/include
          cp lib/libmemalloc.so $out/lib
          cp include/memalloc.h $out/include
        '';
      };

      debug = self.packages.${system}.default.overrideAttrs (oldAttrs: {
        name = "Mem-Alloc (DEBUG)";

        hardeningDisable = [ "fortify" ];

        separateDebugInfo = true;
        enableDebugging = true;

        doCheck = true;

        dontStrip = true;

        NIX_CFLAGS_COMPILE = "-g -O0 -Llib";

        buildPhase = ''
          make DEBUG=1
        '';

        checkPhase = ''
          export LD_LIBRARY_PATH=lib:$LD_LIBRARY_PATH
          make -f ./run-tests.mk
        '';

        installPhase = ''
          mkdir -vp $out/lib $out/include $out/bin
          cp -v lib/libmemalloc.so $out/lib
          cp -v include/memalloc.h $out/include
          cp -v bin/* $out/bin
        '';
      });
    };
    
    devShells.${system}.default = pkgs.mkShell {
      nativeBuildInputs = with pkgs; [
        gcc
        gnumake
        clang-tools
        valgrind
        bear
        gdb
        (pkgs.writeShellScriptBin "make-bear" ''bear -- make'')   
      ];
    };
  };
}
