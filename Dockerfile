FROM centos:centos7 AS base-dependencies
LABEL author="James Cherry"
LABEL maintainer="James Cherry <cherry@parallaxsw.com>"

# Install dev and runtime dependencies
RUN yum group install -y "Development Tools" \
    && yum install -y https://repo.ius.io/ius-release-el7.rpm \
    && yum install -y wget git centos-release-scl devtoolset-8 \
    devtoolset-8-libatomic-devel tcl-devel tcl tk libstdc++ tk-devel pcre-devel \
    yum clean -y all && \
    rm -rf /var/lib/apt/lists/*

# Install CMake
RUN wget https://cmake.org/files/v3.14/cmake-3.14.0-Linux-x86_64.sh && \
    chmod +x cmake-3.14.0-Linux-x86_64.sh  && \
    ./cmake-3.14.0-Linux-x86_64.sh --skip-license --prefix=/usr/local && rm -rf cmake-3.14.0-Linux-x86_64.sh \
    && yum clean -y all

# Install SWIG
RUN yum remove -y swig \
    && wget https://github.com/swig/swig/archive/rel-4.0.1.tar.gz \
    && tar xfz rel-4.0.1.tar.gz \
    && rm -rf rel-4.0.1.tar.gz \
    && cd swig-rel-4.0.1 \
    && ./autogen.sh && ./configure --prefix=/usr && make -j $(nproc) && make install \
    && cd .. \
    && rm -rf swig-rel-4.0.1

# download CUDD
# RUN wget https://www.davidkebo.com/source/cudd_versions/cudd-3.0.0.tar.gz && \
#     tar -xvf cudd-3.0.0.tar.gz && \
#     rm cudd-3.0.0.tar.gz

# install CUDD
# RUN cd cudd-3.0.0 && \
#     mkdir ../cudd && \
#     ./configure --prefix=$HOME/cudd && \
#     make && \
#     make install


FROM base-dependencies AS builder

COPY . /OpenSTA
WORKDIR /OpenSTA

# Build
RUN mkdir build
#RUN cd buld && cmake .. -DCUDD=$HOME/cudd
RUN cd build && cmake ..
RUN make -j 4

# Run sta on entry
ENTRYPOINT ["OpenSTA/app/sta"]
