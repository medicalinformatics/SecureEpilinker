# Secure EpiLinker

Perform privacy-preserving record linkage using the EpiLink algorithm

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.

### Prerequisites

What things you need to install the software and how to install them

```
git
cmake
c++ 17 compatible compiler (gcc >= 8)
boost developement headers (for restbed)
```

### Installing

The buildprocess is not fully automated yet. First you need to clone this
repository (because the usage of submodules *do not* download zip)

```
git@git.compbiol.bio.tu-darmstadt.de:kussel/secure_epilink.git
```

Checkout the Secure EpiLinker branch and initialize the submodules. Depending on
your internet connection this may take some time. After that prepare the build directory for an out-of-code build:

```
cd secure_epilink
git submodule update --init --recursive
mkdir build
```
Restbed needs some custom built OpenSSL, which is not automated (yet).
```
cd extern/restbed/dependency/openssl
./config && make
```
Further more Restbed has some missing link libraries for it's tests in the CMake
Configuration, which need to be patched.
```
cd ../..
git apply ../../cmake/restbed.patch
```
Now the Secure EpiLinker is ready for build. Enter the build directory and
invoce cmake. The project is build with the resulting Makefile.

```
cd ../../build
cmake ..
make
```

For now, the path from which `test_sel` or `sel` is called needs to have a
symlink to the `ABY/bin/circ` folder, where it can find the `int_div_16`
circuit description. E.g. for running from `build/`:

```
ln -s ../extern/ABY/bin/circ/
```

To conclude all steps in one codeblock for convenience:

```
git@git.compbiol.bio.tu-darmstadt.de:kussel/secure_epilink.git
cd secure_epilink && mkdir build
git submodule update --init --recursive
cd extern/restbed/dependency/openssl
./config && make 
cd ../..
git apply ../../cmake/restbed.patch
cd ../../build
cmake .. && make
ln -s ../extern/ABY/bin/circ/
```

We hope to integrate all external builds and patches in the main build process
soon.

## Running the tests

Explain how to run the automated tests for this system

### Break down into end to end tests

Explain what these tests test and why

```
Give an example
```

### And coding style tests

Explain what these tests test and why

```
Give an example
```

## Deployment

Add additional notes about how to deploy this on a live system

## REST interface

Information on how to use the API

## Built With

* [ABY](https://github.com/encryptogroup/ABY/) - The multi party computation framework used
* [Restbed](https://github.com/Corvusoft/restbed/) - The REST Framework used
* [libfmt](https://github.com/fmtlib/fmt/) - Used to format and print text
* [nlohmann/json](https://github.com/nlohmann/json/) - The JSON library used
* [valijson](https://github.com/tristanpenman/valijson) - JSON schema validation library
* [cxxpts](https://github.com/jarro2783/cxxopts/) - Commandline option parser


## Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details on our code of conduct, and the process for submitting pull requests to us.

## Versioning

We use [SemVer](http://semver.org/) for versioning. For the versions available, see the [tags on this repository](https://git.compbiol.bio.tu-darmstadt.de/kussel/secure_epilink/tags). 

## Authors

* **Tobias Kussel** - *Initial work* - Github Link?
* **Sebastian Stammler** - *Initial work* - Github Link?

See also the list of [contributors](https://git.compbiol.bio.tu-darmstadt.de/kussel/secure_epilink/graphs/dev) who participated in this project.

## License

This project is licensed under the AGPL License - see the [LICENSE](LICENSE) file for details

## Acknowledgments

* Thanks to [Lennard Braun](https://github.com/lenerd/ABY-build) for a sane ABY build process
* This work has been supported by the German Federal Ministry of Education and Research (BMBF) and by the Hessian State Ministry for Higher Education, Research and the Arts (HMWK) within the [HiGHmed Consortium](http://www.highmed.org) and [CRISP](http://www.crisp-da.de).

