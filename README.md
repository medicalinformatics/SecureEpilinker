# Secure EpiLinker

Perform privacy-preserving record linkage using the EpiLink algorithm

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.

### Prerequisites

What you need to build and install.

```
git
cmake (>= 3.10)
c++ 17 compatible compiler (gcc >= 8, Ubuntu: g++-8)
boost development headers (for restbed) (Ubuntu: libboost-dev)
libcurl (Ubuntu: libcurl4-openssl-dev)
openssl (Ubuntu: libssl-dev)
gmp (Ubuntu: libgmp-dev)
```

Note that on Arch Linux based systems, openssl is not shipped with static
libraries, so unfortunately the restbed included openssl version needs to
be cloned and built or another custom openssl with static libraries.

### Clone

To clone this repository and the necessary submodules, run

```
git clone git@git.compbiol.bio.tu-darmstadt.de:kussel/secure_epilink.git
cd secure_epilink
scripts/init_submodules.sh
```

Depending on your internet connection this may take some time. Note that the
minimal amount of submodules is cloned and shallow cloning is applied where
possible. You may also clone with `--recurse-submodules` but this will clone
more submodules than necessary.

### Build

#### OpenSSL (only on Arch)

As mentioned above, on Arch you may need to manually compile (static) openssl
libs:
```
cd extern/restbed/dependency/
git submodule update --init openssl
cd openssl
./config && make
```

#### Secure Epilinker

Build it via cmake from the repo top-level directory with
```
mkdir build
cd build
cmake ..
make -j$(nproc) sel
```

Omit or customize `-j` if you don't want to build in parallel with the maximum
amount of threads available on your system.

For now, the path from which `test_sel` or `sel` is called needs to have a
symlink to the `ABY/bin/circ` folder, where it can find the `int_div_16.aby`
circuit description. E.g. for running from `build/`:

```
ln -s ../extern/ABY/bin/circ/
```

To conclude all steps in one code block for convenience:

```
git clone git@git.compbiol.bio.tu-darmstadt.de:kussel/secure_epilink.git
cd secure_epilink
scripts/init_submodules.sh
# START optional on Arch or where static openssl is not available
cd extern/restbed/dependency/openssl
./config && make
cd ../../../..
# END
mkdir build; cd build
cmake .. && make -j $(nproc) sel
ln -s ../extern/ABY/bin/circ/
```

### Keys and Certificates

To run the service in https mode with self-created keys and self-signed
certificates, generate those with `scripts/genkeys.sh`.

## Running the tests

Test targets for different components exist:

  * test_sel
  * test_aby
  * test_util

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

### :whale: Docker
The easiest way to build and deploy the Secure Epilinker is via Docker.
There are two flavors, both being multi-stage Docker builds:

* `Dockerfile.ubuntu`: Build and runtime image are the latest rolling Ubuntu
  release. This is good for debugging the runtime image as you have access to
  the basic tools included in `ubuntu:rolling`

* `Dockerfile.distroless`: uses gcr.io/distroless/cc as the base image for
  minimal size and exposure to security threats. It doesn't contain any binaries
  besides `sel`, so in particular not even `/bin/sh`. Note that currently the
  `cc:debug` flavor is used as the runtime image for debugging purposes. Remove
  the `:debug` tag in the Dockerfile if building for production.

To build the Docker container, clone the repo and its submodules as described
above and run the Docker build:

```
git clone git@git.compbiol.bio.tu-darmstadt.de:kussel/secure_epilink.git
cd secure_epilink
scripts/init_submodules.sh
docker build -t sel:ubuntu -f Dockerfile.ubuntu .
```

When running the image, you need to forward the appropriate ports and configure
the network to allow the image communication access to the local Mainzelliste
and remote Secure Epilinker. Ignoring the network setup, run something similar
to:

```
docker run -p 8161:8161 -p 1337-1344:1337-1344 sel:ubuntu -vv -s
```

This would expose `sel` on its default port on the Docker host and enable
debug output logging and https.

Logs will be printed to standard output as well as be written to the `/log`
directory inside the Docker container.

## REST interface

Information on how to use the API

## Built With

* [ABY](https://github.com/encryptogroup/ABY/) - The multi party computation framework used
* [Restbed](https://github.com/Corvusoft/restbed/) - The REST Framework used
* [libfmt](https://github.com/fmtlib/fmt/) - Used to format and print text
* [nlohmann/json](https://github.com/nlohmann/json/) - The JSON library used
* [valijson](https://github.com/tristanpenman/valijson) - JSON schema validation library
* [cxxpts](https://github.com/jarro2783/cxxopts/) - Commandline option parser
* [curlpp](https://github.com/jpbarrette/curlpp) - HTTP communication

## Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details on our code of conduct, and the process for submitting pull requests to us.

## Versioning

We use [SemVer](http://semver.org/) for versioning. For the versions available, see the [tags on this repository](https://git.compbiol.bio.tu-darmstadt.de/kussel/secure_epilink/tags). 

## Authors

* **Tobias Kussel** - *Initial work* - Github Link?
* **Sebastian Stammler** - *Initial work* - Github Link?
* **Philip Schoppmann_** - _Initial docker build file_ - Github Link?

See also the list of [contributors](https://git.compbiol.bio.tu-darmstadt.de/kussel/secure_epilink/graphs/dev) who participated in this project.

## License

This project is licensed under the AGPL License - see the [LICENSE](LICENSE) file for details

## Acknowledgments

* Thanks to [Lennard Braun](https://github.com/lenerd/ABY-build) for a sane ABY build process
* This work has been supported by the German Federal Ministry of Education and Research (BMBF) and by the Hessian State Ministry for Higher Education, Research and the Arts (HMWK) within the [HiGHmed Consortium](http://www.highmed.org) and [CRISP](http://www.crisp-da.de).

