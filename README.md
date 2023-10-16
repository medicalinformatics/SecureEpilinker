# Mainzelliste SecureEpiLinker (MainSEL)
Privacy-Preserving Record Linkage using Secure Multi-Party Computation and the
EpiLink algorithm. This is the repository of the ABY sMCP Node: SecureEpilinker.
The SEL extended Mainzelliste is available at https://github.com/medicalinformatics/MainzellisteSEL.

## Getting Started

These instructions will get you a copy of the project up and running on your
local machine for development and testing purposes. See deployment for notes on
how to deploy the project on a live system.

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

Note that on Arch Linux based systems, `openssl` is not shipped with static
libraries, so unfortunately the `openssl` version included in `restbed` needs to
be cloned and built or another custom `openssl` with static libraries needs to be
installed.

### Clone

To clone this repository and the necessary submodules, run
```
git clone https://github.com/medicalinformatics/SecureEpilinker.git
cd SecureEpilinker
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
cd extern/restbed/dependency
git submodule update --init openssl
cd openssl
./config && make -j$(nproc)
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

### Running

The default server configuration logs to `../log/`, so create that directory in the
repository's top level folder if running from `build/`. To start the Secure
Epilinker with https enabled and log level debug, run

```
# current directory is build/
mkdir ../log
./sel -s -vv
```

To conclude all steps in one code block for convenience:

```
git clone https://github.com/medicalinformatics/SecureEpilinker.git
cd SecureEpilinker
scripts/init_submodules.sh
# START optional on Arch or where static openssl is not available
cd extern/restbed/dependency/openssl
./config && make -j$(nproc)
cd ../../../..
# END
mkdir build log; cd build
cmake .. && make -j$(nproc) sel
./sel -s -vv
```

### Keys and Certificates

To run the service in https mode with self-created keys and self-signed
certificates, generate those with `scripts/genkeys.sh`.

## Tests

Test build targets for different components exist:

  * `test_sel` to build and run the SEL circuit tests
  * `test_aby` to build and run ABY tests
  * `test_util` to test utility functions

### SEL Tests

To run the SEL circuit tests, the `test_sel` binary needs to be build and
started in two separate terminals, each representing one of two sMPC nodes. An
example of a simple invocation is as follows:

```sh
cd build
make -j $(nproc) test_sel
# Terminal Alice (server)
./test_sel -r 0 -v
# Terminal Bob (client)
./test_sel -r 1 -v
```

Alternatively, using the [tmuxp](https://github.com/tmux-python/tmuxp) utility,
both sessions can be started in a tmux session by invoking
```sh
cd build
tmuxp load ../test/test_sel.tmuxp.yaml
```
This will run the benchmarks and write them to a file. To show the results in
the terminal, use <arrow-up> and change the invocation to
`./test_sel -r $role -v`. Use the `-h` flag to see additional options.

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
git clone https://github.com/medicalinformatics/SecureEpilinker.git
cd SecureEpilinker
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

TODO

## PySEL

The build target `pysel` generates python bindings to some functionalities of
SEL. Currently, only bindings to the clear-text EpiLink calculation are
available. After running `make pysel`, the CMake `build` directory will contain
a python module named `pysel.cpython-38-x86_64-linux-gnu.so`, or similar. This
module can be loaded by any python runtime and used like so:
```python
import pysel

pysel.set_log_level(3) # optionally set log-level (here: warning)

# create input with one record and two database entries, only containing a Bloom
# field "name" of size at most 16 bits (two bytes set in this example).
# Note that this examle will core-dump as the DKFZ EpiLink config is more complex.
input = pysel.Input({"name": [5, 23]}, {"name": [[24, 5], [88, 123]]})
res = pysel.epilink_dkfz_int(input) # perform EpiLinkage

# Or using the multi-record version
records = [{"name": [5, 23]}, {"name": [87, 234]}] # two records
db = {"name": [[24, 5], [89, 123]]} # two database entries
res = pysel.v_epilink_dkfz_int(input) # perform multiple EpiLinkages
```

## Built With

* [ABY](https://github.com/encryptogroup/ABY/) - The multi party computation framework used
* [Restbed](https://github.com/Corvusoft/restbed/) - The REST Framework used
* [libfmt](https://github.com/fmtlib/fmt/) - Used to format and print text
* [nlohmann/json](https://github.com/nlohmann/json/) - The JSON library used
* [valijson](https://github.com/tristanpenman/valijson) - JSON schema validation library
* [cxxpts](https://github.com/jarro2783/cxxopts/) - Commandline option parser
* [curlpp](https://github.com/jpbarrette/curlpp) - HTTP communication
* [pybind11](https://github.com/pybind/pybind11) - Python bindings

## Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details on our code of conduct, and the process for submitting pull requests to us.

## Versioning

We use [SemVer](http://semver.org/) for versioning. For the versions available, see the [tags on this repository](https://git.compbiol.bio.tu-darmstadt.de/kussel/secure_epilink/tags). 

## Authors

* **Tobias Kussel** - *Main author/focus on REST stack* - @tkussel
* **Sebastian Stammler** - *Main author/focus on sMPC circuit* - @sebastianst
* **Phillipp Schoppmann** - _Initial docker build file_ - @schoppmp

## License

This project is licensed under the AGPL License - see the [LICENSE](LICENSE) file for details

## Acknowledgments

* Thanks to [Lennart Braun](https://github.com/lenerd/ABY-build) for a sane ABY build process
* This work has been supported by the German Federal Ministry of Education and Research (BMBF) and by the Hessian State Ministry for Higher Education, Research and the Arts (HMWK) within the [HiGHmed Consortium](http://www.highmed.org) and [CRISP](http://www.crisp-da.de).

