/* Copyright (c) 2002-2007 Jean-Marc Valin
   Copyright (c) 2008 CSIRO
   Copyright (c) 2007-2013 Xiph.Org Foundation
   File: opusdec.c

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef OPUSUTILS_H
#define OPUSUTILS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <opus.h>
#include <opus_multistream.h>

  OpusMSDecoder * process_opus_header(OMX_HANDLETYPE ap_hdl, OMX_U8 * ap_ogg_data,
                                      const OMX_U32 nbytes, opus_int32 *rate,
                                      int *mapping_family, int *channels, int *preskip, float *gain,
                                      float manual_gain, int *streams, int quiet);

  void process_opus_comments(OMX_HANDLETYPE ap_hdl, char *comments, int length);

#ifdef __cplusplus
}
#endif

#endif  /* OPUSUTILS_H */

