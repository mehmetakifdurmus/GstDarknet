/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2020 Niels De Graef <niels.degraef@gmail.com>
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

#ifndef __GST_DARKNET_H__
#define __GST_DARKNET_H__

#include <gst/gst.h>
#include <darknet.h>
G_BEGIN_DECLS

#define GST_TYPE_DARKNET (gst_darknet_get_type())
G_DECLARE_FINAL_TYPE (GstDarknet, gst_darknet,
    GST, PLUGIN_TEMPLATE, GstElement)

#define GST_DARKNET(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_DARKNET,GstDarknet))

struct _GstDarknet
{
  GstElement element;
  GstPad *sinkpad, *srcpad;
  gboolean silent;

  //darknet.so handle
  void *handle;

  //Darknet Function Pointers
  network* (*_load_network)(char *cfg, char *weights, int clear);
  void (*_free_network)(network *net);
  image** (*_load_alphabet)();
  char** (*_get_labels)(char *filename);

  float* (*_network_predict)(network *net, float *input);
  detection* (*_get_network_boxes)(network *net, int w, int h, float thresh, float hier, int *map, int relative, int *num);
  void (*_do_nms_sort)(detection *dets, int total, int classes, float thresh);
  void (*_draw_detections)(image im, detection *dets, int num, float thresh, char **names, image **alphabet, int classes);
  void (*_free_detections)(detection *dets, int n);
  image (*_resize_image)(image im, int w, int h);
  void (*_free_image)(image m);

  //Config files
  char *cfgfile[255];
  char *weightfile[255];
  char *labelfile[255];
  char **labels;
  size_t classes;

  image **alphabet;
  network *net;

  //Image Buffer.
  image img;
};

G_END_DECLS

#endif /* __GST_DARKNET_H__ */
