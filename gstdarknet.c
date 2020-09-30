/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2020 Mehmet Akif Durmus <<user@hostname.org>>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-darknet
 *
 * FIXME:Describe darknet here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! darknet ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include <gst/app/app.h>

#include "gstdarknet.h"
#include <string.h>
#include <dlfcn.h>
#include "immintrin.h"

GST_DEBUG_CATEGORY_STATIC (gst_darknet_debug);
#define GST_CAT_DEFAULT gst_darknet_debug

/* Filter signals and args */
enum
{
    /* FILL ME */
    LAST_SIGNAL
};

enum
{
    PROP_0,
    PROP_SILENT
};

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
                                                                    GST_PAD_SINK,
                                                                    GST_PAD_ALWAYS,
                                                                    GST_STATIC_CAPS ("ANY")
                                                                    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
                                                                   GST_PAD_SRC,
                                                                   GST_PAD_ALWAYS,
                                                                   GST_STATIC_CAPS ("ANY")
                                                                   );

#define gst_darknet_parent_class parent_class
G_DEFINE_TYPE (GstDarknet, gst_darknet, GST_TYPE_ELEMENT);

static void gst_darknet_set_property (GObject * object, guint prop_id,
                                      const GValue * value, GParamSpec * pspec);
static void gst_darknet_get_property (GObject * object, guint prop_id,
                                      GValue * value, GParamSpec * pspec);

static gboolean gst_darknet_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_darknet_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);
static void gst_darknet_finalize (GObject * object);

/* GObject vmethod implementations */

/* initialize the darknet's class */
static void
gst_darknet_class_init (GstDarknetClass * klass)
{
    GObjectClass *gobject_class;
    GstElementClass *gstelement_class;

    gobject_class = (GObjectClass *) klass;
    gstelement_class = (GstElementClass *) klass;

    gobject_class->set_property = gst_darknet_set_property;
    gobject_class->get_property = gst_darknet_get_property;

    g_object_class_install_property (gobject_class, PROP_SILENT,
                                     g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
                                                           FALSE, G_PARAM_READWRITE));

    gst_element_class_set_details_simple(gstelement_class,
                                         "Darknet",
                                         "Neural Networks",
                                         "Neural Network Element",
                                         "Mehmet Akif Durmus github.com/mehmetakifdurmus");

    gst_element_class_add_pad_template (gstelement_class,
                                        gst_static_pad_template_get (&src_factory));
    gst_element_class_add_pad_template (gstelement_class,
                                        gst_static_pad_template_get (&sink_factory));

    gobject_class->finalize = gst_darknet_finalize;
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */

static void
gst_darknet_init (GstDarknet * filter)
{
    filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
    gst_pad_set_event_function (filter->sinkpad,
                                GST_DEBUG_FUNCPTR(gst_darknet_sink_event));
    gst_pad_set_chain_function (filter->sinkpad,
                                GST_DEBUG_FUNCPTR(gst_darknet_chain));
    GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
    gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

    filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
    GST_PAD_SET_PROXY_CAPS (filter->srcpad);
    gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

    filter->silent = FALSE;

    strcpy(filter->cfgfile, "/home/mad/darknet/cfg/yolov3.cfg");
    strcpy(filter->weightfile, "/home/mad/darknet/yolov3.weights");
    strcpy(filter->labelfile, "/home/mad/darknet/data/coco.names");

    //Darknet load
    filter->handle = dlopen("/home/mad/darknet/libdarknet.so", RTLD_LAZY);
    *(void**)(&filter->_load_network) = dlsym(filter->handle, "load_network");
    *(void**)(&filter->_free_network) = dlsym(filter->handle, "free_network");
    *(void**)(&filter->_load_alphabet) = dlsym(filter->handle, "load_alphabet");
    *(void**)(&filter->_network_predict) = dlsym(filter->handle, "network_predict");
    *(void**)(&filter->_get_network_boxes) = dlsym(filter->handle, "get_network_boxes");
    *(void**)(&filter->_do_nms_sort) = dlsym(filter->handle, "do_nms_sort");
    *(void**)(&filter->_draw_detections) = dlsym(filter->handle, "draw_detections");
    *(void**)(&filter->_get_labels) = dlsym(filter->handle, "get_labels");
    *(void**)(&filter->_free_detections) = dlsym(filter->handle, "free_detections");
    *(void**)(&filter->_resize_image) = dlsym(filter->handle, "resize_image");
    *(void**)(&filter->_free_image) = dlsym(filter->handle, "free_image");


    filter->alphabet = filter->_load_alphabet();
    filter->labels = filter->_get_labels(filter->labelfile);
    while (filter->labels[filter->classes] != NULL)
        ++filter->classes;


    filter->net = filter->_load_network(filter->cfgfile, filter->weightfile, NULL);

    filter->img.c = 3;
    filter->img.w = 1280;
    filter->img.h = 720;
    filter->img.data = malloc(sizeof(float) * filter->img.w * filter->img.h * filter->img.c);
}

static void
gst_darknet_finalize (GObject * object)
{
    GstDarknet *filter = GST_DARKNET (object);
    free(filter->img.data);
    filter->_free_network(filter->net);
    free(filter->labels);
    dlclose(filter->handle);
    g_print ("Destructor.\n");
}

static void
gst_darknet_set_property (GObject * object, guint prop_id,
                          const GValue * value, GParamSpec * pspec)
{
    GstDarknet *filter = GST_DARKNET (object);

    switch (prop_id) {
    case PROP_SILENT:
        filter->silent = g_value_get_boolean (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
gst_darknet_get_property (GObject * object, guint prop_id,
                          GValue * value, GParamSpec * pspec)
{
    GstDarknet *filter = GST_DARKNET (object);

    switch (prop_id) {
    case PROP_SILENT:
        g_value_set_boolean (value, filter->silent);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

/* GstElement vmethod implementations */

/* this function handles sink events */
static gboolean
gst_darknet_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
    GstDarknet *filter;
    gboolean ret;

    filter = GST_DARKNET (parent);

    GST_LOG_OBJECT (filter, "Received %s event: %" GST_PTR_FORMAT,
                    GST_EVENT_TYPE_NAME (event), event);

    switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
        GstCaps * caps;

        gst_event_parse_caps (event, &caps);
        /* do something with the caps */

        /* and forward */
        ret = gst_pad_event_default (pad, parent, event);
        break;
    }
    default:
        ret = gst_pad_event_default (pad, parent, event);
        break;
    }
    return ret;
}

typedef struct
{
    guint8 red;
    guint8 green;
    guint8 blue;
} Pixel;

static void yolo_image_to_rgb(image im, guint8* rawimg)
{
    //original formula
    //rawpix[y * im.w + x].green = im.data[c * im.h * im.w + y * im.w + x];
    //small optimizations:
    guint32 size = im.h * im.w; //for green
    guint32 size_2x = 2 * im.h * im.w; // for blue

    Pixel* rawpix = (Pixel*) rawimg;
    for(int y = 0; y < im.h; ++y)
    {
        for(int x = 0; x < im.w; ++x)
        {
            guint32 stride_mul_y = y * im.w;
            rawpix[y * im.w + x].red = im.data[stride_mul_y + x] * 255;
            rawpix[y * im.w + x].green = im.data[stride_mul_y + size + x] * 255;
            rawpix[y * im.w + x].blue = im.data[stride_mul_y + size_2x + x] * 255;
        }
    }
}

static void rgb_to_yolo_image(guint8* rawimg, image* im)
{    
    //width and height should already appended in im
    guint32 size = im->h * im->w; //for green
    guint32 size_2x = 2 * im->h * im->w; // for blue

    Pixel* rawpix = (Pixel*) rawimg;
    for(int y = 0; y < im->h; ++y)
    {
        for(int x = 0; x < im->w; ++x)
        {
            guint32 stride_mul_y = y * im->w;
            im->data[stride_mul_y + x] = rawpix[y * im->w + x].red / 255.;
            im->data[stride_mul_y + size + x] = rawpix[y * im->w + x].green / 255.;
            im->data[stride_mul_y + size_2x + x] = rawpix[y * im->w + x].blue / 255.;
        }
    }       
}

static void wtf(guint8* rawimg)
{
    Pixel* rawpix = (Pixel*) rawimg;

    for(int y = 640; y < 700; ++y)
    for(int x = 0; x < 1280; ++x)
    {
        rawpix[y * 1280 + x].red = 0;
        rawpix[y * 1280 + x].green = 0;
        rawpix[y * 1280 + x].blue = 0;
    }

}


/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_darknet_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
    GstDarknet *filter;

    filter = GST_DARKNET (parent);

    GstMapInfo map;
    gst_buffer_map(buf, &map, GST_MAP_READWRITE);

//    wtf(map.data);


    rgb_to_yolo_image(map.data, &filter->img);

    float nms=.45;
    float hier_thresh = 0.5;
    float thresh = 0.5;

    image sized = filter->_resize_image(filter->img, filter->net->w, filter->net->h);

    filter->_network_predict(filter->net, sized.data);

    int nboxes = 0;
    detection *dets = filter->_get_network_boxes(filter->net, filter->img.w, filter->img.h, thresh, hier_thresh, 0, 1, &nboxes);

    if (nms)
        filter->_do_nms_sort(dets, nboxes, filter->classes, nms);

    filter->_draw_detections(filter->img, dets, nboxes, thresh, filter->labels, filter->alphabet, filter->classes);

    yolo_image_to_rgb(filter->img, map.data);

    for (guint8 i = 0; i < nboxes; ++i) {
        for (guint8 j = 0; j < filter->classes; ++j) {
            if (dets[i].prob[j] > thresh) {
                g_print("%s %d\n", filter->labels[j], (int16_t) (dets[i].prob[j] * 100));
            }
        }
    }

    filter->_free_detections(dets, nboxes);
    filter->_free_image(sized);

    gst_buffer_unmap(buf, &map);

    /* just push out the incoming buffer without touching it */
    return gst_pad_push (filter->srcpad, buf);
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
darknet_init (GstPlugin * darknet)
{
    /* debug category for fltering log messages
   *
   * exchange the string 'Template darknet' with your description
   */

    GST_DEBUG_CATEGORY_INIT (gst_darknet_debug, "darknet",
                             0, "darknetplugin");

    return gst_element_register (darknet, "darknet", GST_RANK_NONE,
                                 GST_TYPE_DARKNET);
}

/* PACKAGE: this is usually set by meson depending on some _INIT macro
 * in meson.build and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use meson to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "darknet"
#endif

/* gstreamer looks for this structure to register darknets
 *
 * exchange the string 'Template darknet' with your darknet description
 */

GST_PLUGIN_DEFINE (
        GST_VERSION_MAJOR,
        GST_VERSION_MINOR,
        darknet,
        "darknet",
        darknet_init,
        "0.1",
        "LGPL",
        "Darknet Plugin For GStreamer",
        "Mehmet Akif Durmus"
        )
