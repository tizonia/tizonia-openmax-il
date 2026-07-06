FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

COPY apt-packages.txt /tmp/tizonia-apt-packages.txt

RUN apt-get update \
    && grep -Ev '^[[:space:]]*(#|$)' /tmp/tizonia-apt-packages.txt \
        | xargs apt-get install -y --no-install-recommends \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /work/tizonia-openmax-il

CMD ["/bin/bash"]
