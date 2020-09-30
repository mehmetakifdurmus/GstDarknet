# GstDarknet

Darknet Neural Network Framework Plugin for GStreamer

It loads darknet in runtime. It is not in final form and is not efficient, so I stopped working on it.

Bug: Does not put rectangles correctly on objects.

Built with QtCreator

Steps:

Download Darknet, and YOLO weights (or etc.) from github.com/pjreddie/darknet.git

Set paths and resolution of video in gst_darknet_init function.

To work with darknets own drawing function add symlink for darknet/data to run folder of gst-launch
eg: ln -s /home/mad/darknet/data /home/mad/data
(It is required for load_alphabet function)

and simply run plugin with:
gst-launch-1.0 --gst-plugin-path=/home/mad/build-GstDarknet-Desktop-Debug/ filesrc location=/home/mad/Videos/cctv2.mp4 ! qtdemux ! avdec_h264 ! videorate ! \
 video/x-raw,framerate=10/1 ! videoconvert ! 'video/x-raw, format=RGB' ! queue ! darknet ! videoconvert ! 'video/x-raw, format=I420' ! autovideosink
