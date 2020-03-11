#!/bin/bash

source ../buildtools.sh

OPCUA_SAMPLER_IMAGE_NAME=intersystemsdc/irisdemo-demo-oee:opcua-sampler-test

docker build -t $OPCUA_SAMPLER_IMAGE_NAME .
exit_if_error "Could not build OPCUA SAMPLER image"

