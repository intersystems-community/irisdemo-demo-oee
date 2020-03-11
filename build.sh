#!/bin/bash
VERSION=`cat ./VERSION`
DOCKER_TAG="version-${VERSION}"

source ./buildtools.sh

OEE_IMAGE_NAME=intersystemsdc/irisdemo-demo-oee:OEE-${DOCKER_TAG}
DISPATCH_IMAGE_NAME=intersystemsdc/irisdemo-demo-oee:DISPATCH-${DOCKER_TAG}
OPCUA_SAMPLER_IMAGE_NAME=intersystemsdc/irisdemo-demo-oee:opcua-sampler-${DOCKER_TAG}

docker build -t $OPCUA_SAMPLER_IMAGE_NAME ./image-opcua-sampler
exit_if_error "Could not build OPCUA SAMPLER image"

# docker build -t $OEE_IMAGE_NAME ./OEE
# exit_if_error "Could not build OEE image"

# docker build -t $DISPATCH_IMAGE_NAME ./DISPATCH
# exit_if_error "Could not build DISPATCH image"
