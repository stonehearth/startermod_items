Steps:

1) Create and push your module repo (e.g. protobuf-2.5.0)
2) Add the submodule: git submodule add https://github.com/radent/protobuf-2.5.0.git modules/protobuf-2.5.0
3) Edit .gitmodules and add 'ignore = dirty' to your new module
4) Verify the path for your module uses unix paths (e.g. modules/foo instead of modules\\foo)
4) Commit...
   git add .gitmodules
   git commit
