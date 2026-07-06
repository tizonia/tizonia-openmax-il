# syntax=docker/dockerfile:1

FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive
ARG TIZONIA_INSTALL_DOCS=false

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

COPY apt-packages.txt /tmp/tizonia-apt-packages.txt

RUN apt-get update \
    && sed -e '/^[[:space:]]*#/d' -e '/^[[:space:]]*$/d' /tmp/tizonia-apt-packages.txt \
       | xargs apt-get install -y --no-install-recommends \
    && if [ "${TIZONIA_INSTALL_DOCS}" = "true" ]; then \
         apt-get install -y --no-install-recommends \
           doxygen \
           graphviz \
           plantuml \
           python3-alabaster \
           python3-breathe \
           python3-recommonmark \
           python3-sphinxcontrib.plantuml \
           python3-sphinx; \
       fi \
    && rm -rf /var/lib/apt/lists/* /tmp/tizonia-apt-packages.txt

WORKDIR /work/tizonia-openmax-il

CMD ["/bin/bash"]
