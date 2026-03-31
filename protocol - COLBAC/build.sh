#!/bin/bash

DOCKERCMD=${DOCKER:-docker}
BASENAME=mr-impl
CANARY=.container_run

$DOCKERCMD container stop  ${BASENAME}-container
$DOCKERCMD container rm    ${BASENAME}-container
$DOCKERCMD rmi             ${BASENAME}-image

if [ -f "${BASENAME}-image.tar" ]; then
	$DOCKERCMD load < ${BASENAME}-image.tar
else
	$DOCKERCMD build --platform=linux/amd64 -t ${BASENAME}-image .
fi
rm -f $CANARY
