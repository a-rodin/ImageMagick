/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%               CCCC   OOO   M   M  PPPP    AAA   RRRR    EEEEE               %
%              C      O   O  MM MM  P   P  A   A  R   R   E                   %
%              C      O   O  M M M  PPPP   AAAAA  RRRR    EEE                 %
%              C      O   O  M   M  P      A   A  R R     E                   %
%               CCCC   OOO   M   M  P      A   A  R  R    EEEEE               %
%                                                                             %
%                                                                             %
%                      MagickCore Image Comparison Methods                    %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                               December 2003                                 %
%                                                                             %
%                                                                             %
%  Copyright 1999-2009 ImageMagick Studio LLC, a non-profit organization      %
%  dedicated to making software imaging solutions freely available.           %
%                                                                             %
%  You may not use this file except in compliance with the License.  You may  %
%  obtain a copy of the License at                                            %
%                                                                             %
%    http://www.imagemagick.org/script/license.php                            %
%                                                                             %
%  Unless required by applicable law or agreed to in writing, software        %
%  distributed under the License is distributed on an "AS IS" BASIS,          %
%  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   %
%  See the License for the specific language governing permissions and        %
%  limitations under the License.                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
%
*/

/*
  Include declarations.
*/
#include "magick/studio.h"
#include "magick/artifact.h"
#include "magick/cache-view.h"
#include "magick/client.h"
#include "magick/color.h"
#include "magick/color-private.h"
#include "magick/colorspace.h"
#include "magick/colorspace-private.h"
#include "magick/compare.h"
#include "magick/composite-private.h"
#include "magick/constitute.h"
#include "magick/exception-private.h"
#include "magick/geometry.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/log.h"
#include "magick/memory_.h"
#include "magick/option.h"
#include "magick/pixel-private.h"
#include "magick/resource_.h"
#include "magick/string_.h"
#include "magick/utility.h"
#include "magick/version.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o m p a r e I m a g e C h a n n e l s                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CompareImageChannels() compares one or more image channels of an image
%  to a reconstructed image and returns the difference image.
%
%  The format of the CompareImageChannels method is:
%
%      Image *CompareImageChannels(const Image *image,
%        const Image *reconstruct_image,const ChannelType channel,
%        const MetricType metric,double *distortion,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o reconstruct_image: the reconstruct image.
%
%    o channel: the channel.
%
%    o metric: the metric.
%
%    o distortion: the computed distortion between the images.
%
%    o exception: return any errors or warnings in this structure.
%
*/

MagickExport Image *CompareImages(Image *image,const Image *reconstruct_image,
  const MetricType metric,double *distortion,ExceptionInfo *exception)
{
  Image
    *highlight_image;

  highlight_image=CompareImageChannels(image,reconstruct_image,AllChannels,
    metric,distortion,exception);
  return(highlight_image);
}

MagickExport Image *CompareImageChannels(Image *image,
  const Image *reconstruct_image,const ChannelType channel,
  const MetricType metric,double *distortion,ExceptionInfo *exception)
{
  const char
    *artifact;

  Image
    *difference_image,
    *highlight_image;

  long
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    highlight,
    lowlight,
    zero;

  ViewInfo
    *highlight_view,
    *image_view,
    *reconstruct_view;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(reconstruct_image != (const Image *) NULL);
  assert(reconstruct_image->signature == MagickSignature);
  assert(distortion != (double *) NULL);
  *distortion=0.0;
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if ((reconstruct_image->columns != image->columns) ||
      (reconstruct_image->rows != image->rows))
    ThrowImageException(ImageError,"ImageSizeDiffers");
  status=GetImageChannelDistortion(image,reconstruct_image,channel,metric,
    distortion,exception);
  if (status == MagickFalse)
    return((Image *) NULL);
  difference_image=CloneImage(image,0,0,MagickTrue,exception);
  if (difference_image == (Image *) NULL)
    return((Image *) NULL);
  (void) SetImageAlphaChannel(difference_image,OpaqueAlphaChannel);
  highlight_image=CloneImage(image,image->columns,image->rows,MagickTrue,
    exception);
  if (highlight_image == (Image *) NULL)
    {
      difference_image=DestroyImage(difference_image);
      return((Image *) NULL);
    }
  if (SetImageStorageClass(highlight_image,DirectClass) == MagickFalse)
    {
      InheritException(exception,&highlight_image->exception);
      difference_image=DestroyImage(difference_image);
      highlight_image=DestroyImage(highlight_image);
      return((Image *) NULL);
    }
  (void) SetImageAlphaChannel(highlight_image,OpaqueAlphaChannel);
  (void) QueryMagickColor("#f1001ecc",&highlight,exception);
  artifact=GetImageArtifact(image,"highlight-color");
  if (artifact != (const char *) NULL)
    (void) QueryMagickColor(artifact,&highlight,exception);
  (void) QueryMagickColor("#ffffffcc",&lowlight,exception);
  artifact=GetImageArtifact(image,"lowlight-color");
  if (artifact != (const char *) NULL)
    (void) QueryMagickColor(artifact,&lowlight,exception);
  if (highlight_image->colorspace == CMYKColorspace)
    {
      ConvertRGBToCMYK(&highlight);
      ConvertRGBToCMYK(&lowlight);
    }
  /*
    Generate difference image.
  */
  status=MagickTrue;
  GetMagickPixelPacket(image,&zero);
  image_view=AcquireCacheView(image);
  reconstruct_view=AcquireCacheView(reconstruct_image);
  highlight_view=AcquireCacheView(highlight_image);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(dynamic,4) shared(status)
#endif
  for (y=0; y < (long) image->rows; y++)
  {
    MagickBooleanType
      sync;

    MagickPixelPacket
      pixel,
      reconstruct_pixel;

    register const IndexPacket
      *indexes,
      *reconstruct_indexes;

    register const PixelPacket
      *p,
      *q;

    register IndexPacket
      *highlight_indexes;

    register long
      x;

    register PixelPacket
      *r;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    q=GetCacheViewVirtualPixels(reconstruct_view,0,y,reconstruct_image->columns,
      1,exception);
    r=QueueCacheViewAuthenticPixels(highlight_view,0,y,highlight_image->columns,
      1,exception);
    if ((p == (const PixelPacket *) NULL) ||
        (q == (const PixelPacket *) NULL) || (r == (PixelPacket *) NULL))
      {
        status=MagickFalse;
        continue;
      }
    indexes=GetCacheViewVirtualIndexQueue(image_view);
    reconstruct_indexes=GetCacheViewVirtualIndexQueue(reconstruct_view);
    highlight_indexes=GetCacheViewAuthenticIndexQueue(highlight_view);
    pixel=zero;
    reconstruct_pixel=zero;
    for (x=0; x < (long) image->columns; x++)
    {
      MagickStatusType
        difference;

      SetMagickPixelPacket(image,p,indexes+x,&pixel);
      SetMagickPixelPacket(reconstruct_image,q,reconstruct_indexes+x,
        &reconstruct_pixel);
      difference=MagickFalse;
      if (channel == AllChannels)
        {
          if (IsMagickColorSimilar(&pixel,&reconstruct_pixel) == MagickFalse)
            difference=MagickTrue;
        }
      else
        {
          if (((channel & RedChannel) != 0) && (p->red != q->red))
            difference=MagickTrue;
          if (((channel & GreenChannel) != 0) && (p->green != q->green))
            difference=MagickTrue;
          if (((channel & BlueChannel) != 0) && (p->blue != q->blue))
            difference=MagickTrue;
          if (((channel & OpacityChannel) != 0) &&
              (image->matte != MagickFalse) && (p->opacity != q->opacity))
            difference=MagickTrue;
          if ((((channel & IndexChannel) != 0) &&
               (image->colorspace == CMYKColorspace) &&
               (reconstruct_image->colorspace == CMYKColorspace)) &&
              (indexes[x] != reconstruct_indexes[x]))
            difference=MagickTrue;
        }
      if (difference != MagickFalse)
        SetPixelPacket(highlight_image,&highlight,r,highlight_indexes+x);
      else
        SetPixelPacket(highlight_image,&lowlight,r,highlight_indexes+x);
      p++;
      q++;
      r++;
    }
    sync=SyncCacheViewAuthenticPixels(highlight_view,exception);
    if (sync == MagickFalse)
      status=MagickFalse;
  }
  highlight_view=DestroyCacheView(highlight_view);
  reconstruct_view=DestroyCacheView(reconstruct_view);
  image_view=DestroyCacheView(image_view);
  (void) CompositeImage(difference_image,image->compose,highlight_image,0,0);
  highlight_image=DestroyImage(highlight_image);
  if (status == MagickFalse)
    difference_image=DestroyImage(difference_image);
  return(difference_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e C h a n n e l D i s t o r t i o n                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageChannelDistortion() compares one or more image channels of an image
%  to a reconstructed image and returns the specified distortion metric.
%
%  The format of the CompareImageChannels method is:
%
%      MagickBooleanType GetImageChannelDistortion(const Image *image,
%        const Image *reconstruct_image,const ChannelType channel,
%        const MetricType metric,double *distortion,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o reconstruct_image: the reconstruct image.
%
%    o channel: the channel.
%
%    o metric: the metric.
%
%    o distortion: the computed distortion between the images.
%
%    o exception: return any errors or warnings in this structure.
%
*/

MagickExport MagickBooleanType GetImageDistortion(Image *image,
  const Image *reconstruct_image,const MetricType metric,double *distortion,
  ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  status=GetImageChannelDistortion(image,reconstruct_image,AllChannels,
    metric,distortion,exception);
  return(status);
}

static MagickBooleanType GetAbsoluteError(const Image *image,
  const Image *reconstruct_image,const ChannelType channel,double *distortion,
  ExceptionInfo *exception)
{
  long
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    zero;

  ViewInfo
    *image_view,
    *reconstruct_view;

  /*
    Compute the absolute difference in pixels between two images.
  */
  status=MagickTrue;
  GetMagickPixelPacket(image,&zero);
  image_view=AcquireCacheView(image);
  reconstruct_view=AcquireCacheView(reconstruct_image);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(dynamic,4) shared(status)
#endif
  for (y=0; y < (long) image->rows; y++)
  {
    double
      channel_distortion[AllChannels+1];

    MagickPixelPacket
      pixel,
      reconstruct_pixel;

    register const IndexPacket
      *indexes,
      *reconstruct_indexes;

    register const PixelPacket
      *p,
      *q;

    register long
      i,
      x;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    q=GetCacheViewVirtualPixels(reconstruct_view,0,y,reconstruct_image->columns,
      1,exception);
    if ((p == (const PixelPacket *) NULL) || (q == (const PixelPacket *) NULL))
      {
        status=MagickFalse;
        continue;
      }
    indexes=GetCacheViewVirtualIndexQueue(image_view);
    reconstruct_indexes=GetCacheViewVirtualIndexQueue(reconstruct_view);
    pixel=zero;
    reconstruct_pixel=pixel;
    (void) ResetMagickMemory(channel_distortion,0,sizeof(channel_distortion));
    for (x=0; x < (long) image->columns; x++)
    {
      SetMagickPixelPacket(image,p,indexes+x,&pixel);
      SetMagickPixelPacket(reconstruct_image,q,reconstruct_indexes+x,
        &reconstruct_pixel);
      if (IsMagickColorSimilar(&pixel,&reconstruct_pixel) == MagickFalse)
        {
          if ((channel & RedChannel) != 0)
            channel_distortion[RedChannel]++;
          if ((channel & GreenChannel) != 0)
            channel_distortion[GreenChannel]++;
          if ((channel & BlueChannel) != 0)
            channel_distortion[BlueChannel]++;
          if (((channel & OpacityChannel) != 0) &&
              (image->matte != MagickFalse))
            channel_distortion[OpacityChannel]++;
          if (((channel & IndexChannel) != 0) &&
              (image->colorspace == CMYKColorspace))
            channel_distortion[BlackChannel]++;
          channel_distortion[AllChannels]++;
        }
      p++;
      q++;
    }
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp critical
#endif
    for (i=0; i <= (long) AllChannels; i++)
      distortion[i]+=channel_distortion[i];
  }
  reconstruct_view=DestroyCacheView(reconstruct_view);
  image_view=DestroyCacheView(image_view);
  return(status);
}

static unsigned long GetNumberChannels(const Image *image,
  const ChannelType channel)
{
  unsigned long
    channels;

  channels=0;
  if ((channel & RedChannel) != 0)
    channels++;
  if ((channel & GreenChannel) != 0)
    channels++;
  if ((channel & BlueChannel) != 0)
    channels++;
  if (((channel & OpacityChannel) != 0) &&
       (image->matte != MagickFalse))
    channels++;
  if (((channel & IndexChannel) != 0) &&
      (image->colorspace == CMYKColorspace))
    channels++;
  return(channels);
}

static MagickBooleanType GetMeanAbsoluteError(const Image *image,
  const Image *reconstruct_image,const ChannelType channel,
  double *distortion,ExceptionInfo *exception)
{
  double
    scale;

  long
    y;

  MagickBooleanType
    status;

  ViewInfo
    *image_view,
    *reconstruct_view;

  status=MagickTrue;
  scale=1.0/((double) image->columns*image->rows);
  image_view=AcquireCacheView(image);
  reconstruct_view=AcquireCacheView(reconstruct_image);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(dynamic,4) shared(status)
#endif
  for (y=0; y < (long) image->rows; y++)
  {
    double
      channel_distortion[AllChannels+1];

    register const IndexPacket
      *indexes,
      *reconstruct_indexes;

    register const PixelPacket
      *p,
      *q;

    register long
      i,
      x;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    q=GetCacheViewVirtualPixels(reconstruct_view,0,y,
      reconstruct_image->columns,1,exception);
    if ((p == (const PixelPacket *) NULL) || (q == (const PixelPacket *) NULL))
      {
        status=MagickFalse;
        continue;
      }
    indexes=GetCacheViewVirtualIndexQueue(image_view);
    reconstruct_indexes=GetCacheViewVirtualIndexQueue(reconstruct_view);
    (void) ResetMagickMemory(channel_distortion,0,sizeof(channel_distortion));
    for (x=0; x < (long) image->columns; x++)
    {
      MagickRealType
        distance;

      if ((channel & RedChannel) != 0)
        {
          distance=fabs(p->red-(double) q->red);
          channel_distortion[RedChannel]+=scale*distance;
          channel_distortion[AllChannels]+=scale*distance;
        }
      if ((channel & GreenChannel) != 0)
        {
          distance=fabs(p->green-(double) q->green);
          channel_distortion[GreenChannel]+=scale*distance;
          channel_distortion[AllChannels]+=scale*distance;
        }
      if ((channel & BlueChannel) != 0)
        {
          distance=fabs(p->blue-(double) q->blue);
          channel_distortion[BlueChannel]+=scale*distance;
          channel_distortion[AllChannels]+=scale*distance;
        }
      if (((channel & OpacityChannel) != 0) &&
          (image->matte != MagickFalse))
        {
          distance=fabs(p->opacity-(double) q->opacity);
          channel_distortion[OpacityChannel]+=scale*distance;
          channel_distortion[AllChannels]+=scale*distance;
        }
      if (((channel & IndexChannel) != 0) &&
          (image->colorspace == CMYKColorspace))
        {
          distance=fabs(indexes[x]-(double) reconstruct_indexes[x]);
          channel_distortion[BlackChannel]+=scale*distance;
          channel_distortion[AllChannels]+=scale*distance;
        }
      p++;
      q++;
    }
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp critical
#endif
    for (i=0; i <= (long) AllChannels; i++)
      distortion[i]+=channel_distortion[i];
  }
  reconstruct_view=DestroyCacheView(reconstruct_view);
  image_view=DestroyCacheView(image_view);
  distortion[AllChannels]/=(double) GetNumberChannels(image,channel);
  return(status);
}

static MagickBooleanType GetMeanErrorPerPixel(Image *image,
  const Image *reconstruct_image,const ChannelType channel,double *distortion,
  ExceptionInfo *exception)
{
  long
    y;

  MagickBooleanType
    status;

  MagickRealType
    alpha,
    area,
    beta,
    maximum_error,
    mean_error;

  ViewInfo
    *image_view,
    *reconstruct_view;

  status=MagickTrue;
  alpha=1.0;
  beta=1.0;
  area=0.0;
  maximum_error=0.0;
  mean_error=0.0;
  image_view=AcquireCacheView(image);
  reconstruct_view=AcquireCacheView(reconstruct_image);
  for (y=0; y < (long) image->rows; y++)
  {
    register const IndexPacket
      *indexes,
      *reconstruct_indexes;

    register const PixelPacket
      *p,
      *q;

    register long
      x;

    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    q=GetCacheViewVirtualPixels(reconstruct_view,0,y,reconstruct_image->columns,1,
      exception);
    if ((p == (const PixelPacket *) NULL) || (q == (const PixelPacket *) NULL))
      {
        status=MagickFalse;
        break;
      }
    indexes=GetCacheViewVirtualIndexQueue(image_view);
    reconstruct_indexes=GetCacheViewVirtualIndexQueue(reconstruct_view);
    for (x=0; x < (long) image->columns; x++)
    {
      MagickRealType
        distance;

      if ((channel & OpacityChannel) != 0)
        {
          if (image->matte != MagickFalse)
            alpha=(MagickRealType) (QuantumScale*(QuantumRange-p->opacity));
          if (reconstruct_image->matte != MagickFalse)
            beta=(MagickRealType) (QuantumScale*(QuantumRange-q->opacity));
        }
      if ((channel & RedChannel) != 0)
        {
          distance=fabs(alpha*p->red-beta*q->red);
          distortion[RedChannel]+=distance;
          distortion[AllChannels]+=distance;
          mean_error+=distance*distance;
          if (distance > maximum_error)
            maximum_error=distance;
          area++;
        }
      if ((channel & GreenChannel) != 0)
        {
          distance=fabs(alpha*p->green-beta*q->green);
          distortion[GreenChannel]+=distance;
          distortion[AllChannels]+=distance;
          mean_error+=distance*distance;
          if (distance > maximum_error)
            maximum_error=distance;
          area++;
        }
      if ((channel & BlueChannel) != 0)
        {
          distance=fabs(alpha*p->blue-beta*q->blue);
          distortion[BlueChannel]+=distance;
          distortion[AllChannels]+=distance;
          mean_error+=distance*distance;
          if (distance > maximum_error)
            maximum_error=distance;
          area++;
        }
      if (((channel & OpacityChannel) != 0) &&
          (image->matte != MagickFalse))
        {
          distance=fabs((double) p->opacity-q->opacity);
          distortion[OpacityChannel]+=distance;
          distortion[AllChannels]+=distance;
          mean_error+=distance*distance;
          if (distance > maximum_error)
            maximum_error=distance;
          area++;
        }
      if (((channel & IndexChannel) != 0) &&
          (image->colorspace == CMYKColorspace) &&
          (reconstruct_image->colorspace == CMYKColorspace))
        {
          distance=fabs(alpha*indexes[x]-beta*reconstruct_indexes[x]);
          distortion[BlackChannel]+=distance;
          distortion[AllChannels]+=distance;
          mean_error+=distance*distance;
          if (distance > maximum_error)
            maximum_error=distance;
          area++;
        }
      p++;
      q++;
    }
  }
  reconstruct_view=DestroyCacheView(reconstruct_view);
  image_view=DestroyCacheView(image_view);
  image->error.mean_error_per_pixel=distortion[AllChannels]/area;
  image->error.normalized_mean_error=QuantumScale*QuantumScale*mean_error/area;
  image->error.normalized_maximum_error=QuantumScale*maximum_error;
  return(status);
}

static MagickBooleanType GetMeanSquaredError(const Image *image,
  const Image *reconstruct_image,const ChannelType channel,
  double *distortion,ExceptionInfo *exception)
{
  double
    scale;

  long
    y;

  MagickBooleanType
    status;

  ViewInfo
    *image_view,
    *reconstruct_view;

  status=MagickTrue;
  scale=QuantumScale/((double) image->columns*image->rows);
  image_view=AcquireCacheView(image);
  reconstruct_view=AcquireCacheView(reconstruct_image);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(dynamic,4) shared(status)
#endif
  for (y=0; y < (long) image->rows; y++)
  {
    double
      channel_distortion[AllChannels+1];

    register const IndexPacket
      *indexes,
      *reconstruct_indexes;

    register const PixelPacket
      *p,
      *q;

    register long
      i,
      x;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    q=GetCacheViewVirtualPixels(reconstruct_view,0,y,
      reconstruct_image->columns,1,exception);
    if ((p == (const PixelPacket *) NULL) || (q == (const PixelPacket *) NULL))
      {
        status=MagickFalse;
        continue;
      }
    indexes=GetCacheViewVirtualIndexQueue(image_view);
    reconstruct_indexes=GetCacheViewVirtualIndexQueue(reconstruct_view);
    (void) ResetMagickMemory(channel_distortion,0,sizeof(channel_distortion));
    for (x=0; x < (long) image->columns; x++)
    {
      MagickRealType
        distance;

      if ((channel & RedChannel) != 0)
        {
          distance=(p->red-(MagickRealType) q->red);
          channel_distortion[RedChannel]+=scale*distance*distance;
          channel_distortion[AllChannels]+=scale*distance*distance;
        }
      if ((channel & GreenChannel) != 0)
        {
          distance=(p->green-(MagickRealType) q->green);
          channel_distortion[GreenChannel]+=scale*distance*distance;
          channel_distortion[AllChannels]+=scale*distance*distance;
        }
      if ((channel & BlueChannel) != 0)
        {
          distance=(p->blue-(MagickRealType) q->blue);
          channel_distortion[BlueChannel]+=scale*distance*distance;
          channel_distortion[AllChannels]+=scale*distance*distance;
        }
      if (((channel & OpacityChannel) != 0) &&
          (image->matte != MagickFalse))
        {
          distance=(p->opacity-(MagickRealType) q->opacity);
          channel_distortion[OpacityChannel]+=scale*distance*distance;
          channel_distortion[AllChannels]+=scale*distance*distance;
        }
      if (((channel & IndexChannel) != 0) &&
          (image->colorspace == CMYKColorspace) &&
          (reconstruct_image->colorspace == CMYKColorspace))
        {
          distance=(indexes[x]-(MagickRealType) reconstruct_indexes[x]);
          channel_distortion[BlackChannel]+=scale*distance*distance;
          channel_distortion[AllChannels]+=scale*distance*distance;
        }
      p++;
      q++;
    }
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp critical
#endif
    for (i=0; i <= (long) AllChannels; i++)
      distortion[i]+=channel_distortion[i];
  }
  reconstruct_view=DestroyCacheView(reconstruct_view);
  image_view=DestroyCacheView(image_view);
  distortion[AllChannels]/=(double) GetNumberChannels(image,channel);
  return(status);
}

static MagickBooleanType GetPeakAbsoluteError(const Image *image,
  const Image *reconstruct_image,const ChannelType channel,
  double *distortion,ExceptionInfo *exception)
{
  long
    y;

  MagickBooleanType
    status;

  ViewInfo
    *image_view,
    *reconstruct_view;

  status=MagickTrue;
  image_view=AcquireCacheView(image);
  reconstruct_view=AcquireCacheView(reconstruct_image);
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp parallel for schedule(dynamic,4) shared(status)
#endif
  for (y=0; y < (long) image->rows; y++)
  {
    double
      channel_distortion[AllChannels+1];

    register const IndexPacket
      *indexes,
      *reconstruct_indexes;

    register const PixelPacket
      *p,
      *q;

    register long
      i,
      x;

    if (status == MagickFalse)
      continue;
    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,exception);
    q=GetCacheViewVirtualPixels(reconstruct_view,0,y,
      reconstruct_image->columns,1,exception);
    if ((p == (const PixelPacket *) NULL) || (q == (const PixelPacket *) NULL))
      {
        status=MagickFalse;
        continue;
      }
    indexes=GetCacheViewVirtualIndexQueue(image_view);
    reconstruct_indexes=GetCacheViewVirtualIndexQueue(reconstruct_view);
    (void) ResetMagickMemory(channel_distortion,0,sizeof(channel_distortion));
    for (x=0; x < (long) image->columns; x++)
    {
      MagickRealType
        distance;

      if ((channel & RedChannel) != 0)
        {
          distance=fabs(p->red-(double) q->red);
          if (distance > channel_distortion[RedChannel])
            channel_distortion[RedChannel]=distance;
          if (distance > channel_distortion[AllChannels])
            channel_distortion[AllChannels]=distance;
        }
      if ((channel & GreenChannel) != 0)
        {
          distance=fabs(p->green-(double) q->green);
          if (distance > channel_distortion[GreenChannel])
            channel_distortion[GreenChannel]=distance;
          if (distance > channel_distortion[AllChannels])
            channel_distortion[AllChannels]=distance;
        }
      if ((channel & BlueChannel) != 0)
        {
          distance=fabs(p->blue-(double) q->blue);
          if (distance > channel_distortion[BlueChannel])
            channel_distortion[BlueChannel]=distance;
          if (distance > channel_distortion[AllChannels])
            channel_distortion[AllChannels]=distance;
        }
      if (((channel & OpacityChannel) != 0) &&
          (image->matte != MagickFalse))
        {
          distance=fabs(p->opacity-(double) q->opacity);
          if (distance > channel_distortion[OpacityChannel])
            channel_distortion[OpacityChannel]=distance;
          if (distance > channel_distortion[AllChannels])
            channel_distortion[AllChannels]=distance;
        }
      if (((channel & IndexChannel) != 0) &&
          (image->colorspace == CMYKColorspace) &&
          (reconstruct_image->colorspace == CMYKColorspace))
        {
          distance=fabs(indexes[x]-(double) reconstruct_indexes[x]);
          if (distance > channel_distortion[BlackChannel])
            channel_distortion[BlackChannel]=distance;
          if (distance > channel_distortion[AllChannels])
            channel_distortion[AllChannels]=distance;
        }
      p++;
      q++;
    }
#if defined(MAGICKCORE_OPENMP_SUPPORT)
  #pragma omp critical
#endif
    for (i=0; i <= (long) AllChannels; i++)
      if (channel_distortion[i] > distortion[i])
        distortion[i]=channel_distortion[i];
  }
  reconstruct_view=DestroyCacheView(reconstruct_view);
  image_view=DestroyCacheView(image_view);
  return(status);
}

static MagickBooleanType GetPeakSignalToNoiseRatio(const Image *image,
  const Image *reconstruct_image,const ChannelType channel,
  double *distortion,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  status=GetMeanSquaredError(image,reconstruct_image,channel,distortion,
    exception);
  if ((channel & RedChannel) != 0)
    distortion[RedChannel]=20.0*log10((double) 1.0/sqrt(QuantumScale*
      distortion[RedChannel]));
  if ((channel & GreenChannel) != 0)
    distortion[GreenChannel]=20.0*log10((double) 1.0/sqrt(QuantumScale*
      distortion[GreenChannel]));
  if ((channel & BlueChannel) != 0)
    distortion[BlueChannel]=20.0*log10((double) 1.0/sqrt(QuantumScale*
      distortion[BlueChannel]));
  if (((channel & OpacityChannel) != 0) &&
      (image->matte != MagickFalse))
    distortion[OpacityChannel]=20.0*log10((double) 1.0/sqrt(QuantumScale*
      distortion[OpacityChannel]));
  if (((channel & IndexChannel) != 0) &&
      (image->colorspace == CMYKColorspace))
    distortion[BlackChannel]=20.0*log10((double) 1.0/sqrt(QuantumScale*
      distortion[BlackChannel]));
  distortion[AllChannels]=20.0*log10((double) 1.0/sqrt(QuantumScale*
    distortion[AllChannels]));
  return(status);
}

static MagickBooleanType GetRootMeanSquaredError(const Image *image,
  const Image *reconstruct_image,const ChannelType channel,
  double *distortion,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  status=GetMeanSquaredError(image,reconstruct_image,channel,distortion,
    exception);
  if ((channel & RedChannel) != 0)
    distortion[RedChannel]=sqrt(QuantumRange*distortion[RedChannel]);
  if ((channel & GreenChannel) != 0)
    distortion[GreenChannel]=sqrt(QuantumRange*distortion[GreenChannel]);
  if ((channel & BlueChannel) != 0)
    distortion[BlueChannel]=sqrt(QuantumRange*distortion[BlueChannel]);
  if (((channel & OpacityChannel) != 0) &&
      (image->matte != MagickFalse))
    distortion[OpacityChannel]=sqrt(QuantumRange*distortion[OpacityChannel]);
  if (((channel & IndexChannel) != 0) &&
      (image->colorspace == CMYKColorspace))
    distortion[BlackChannel]=sqrt(QuantumRange*distortion[BlackChannel]);
  distortion[AllChannels]=sqrt(QuantumRange*distortion[AllChannels]);
  return(status);
}

MagickExport MagickBooleanType GetImageChannelDistortion(Image *image,
  const Image *reconstruct_image,const ChannelType channel,
  const MetricType metric,double *distortion,ExceptionInfo *exception)
{
  double
    *channel_distortion;

  MagickBooleanType
    status;

  size_t
    length;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(reconstruct_image != (const Image *) NULL);
  assert(reconstruct_image->signature == MagickSignature);
  assert(distortion != (double *) NULL);
  *distortion=0.0;
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if ((reconstruct_image->columns != image->columns) ||
      (reconstruct_image->rows != image->rows))
    ThrowBinaryException(ImageError,"ImageSizeDiffers",image->filename);
  /*
    Get image distortion.
  */
  length=AllChannels+1UL;
  channel_distortion=(double *) AcquireQuantumMemory(length,
    sizeof(*channel_distortion));
  if (channel_distortion == (double *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  (void) ResetMagickMemory(channel_distortion,0,length*
    sizeof(*channel_distortion));
  switch (metric)
  {
    case AbsoluteErrorMetric:
    {
      status=GetAbsoluteError(image,reconstruct_image,channel,
        channel_distortion,exception);
      break;
    }
    case MeanAbsoluteErrorMetric:
    {
      status=GetMeanAbsoluteError(image,reconstruct_image,channel,
        channel_distortion,exception);
      break;
    }
    case MeanErrorPerPixelMetric:
    {
      status=GetMeanErrorPerPixel(image,reconstruct_image,channel,
        channel_distortion,exception);
      break;
    }
    case MeanSquaredErrorMetric:
    {
      status=GetMeanSquaredError(image,reconstruct_image,channel,
        channel_distortion,exception);
      break;
    }
    case PeakAbsoluteErrorMetric:
    default:
    {
      status=GetPeakAbsoluteError(image,reconstruct_image,channel,
        channel_distortion,exception);
      break;
    }
    case PeakSignalToNoiseRatioMetric:
    {
      status=GetPeakSignalToNoiseRatio(image,reconstruct_image,channel,
        channel_distortion,exception);
      break;
    }
    case RootMeanSquaredErrorMetric:
    {
      status=GetRootMeanSquaredError(image,reconstruct_image,channel,
        channel_distortion,exception);
      break;
    }
  }
  *distortion=channel_distortion[AllChannels];
  channel_distortion=(double *) RelinquishMagickMemory(channel_distortion);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e C h a n n e l D i s t o r t i o n s                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageChannelDistrortion() compares the image channels of an image to a
%  reconstructed image and returns the specified distortion metric for each
%  channel.
%
%  The format of the CompareImageChannels method is:
%
%      double *GetImageChannelDistortions(const Image *image,
%        const Image *reconstruct_image,const MetricType metric,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: the image.
%
%    o reconstruct_image: the reconstruct image.
%
%    o metric: the metric.
%
%    o exception: return any errors or warnings in this structure.
%
*/
MagickExport double *GetImageChannelDistortions(Image *image,
  const Image *reconstruct_image,const MetricType metric,
  ExceptionInfo *exception)
{
  double
    *channel_distortion;

  MagickBooleanType
    status;

  size_t
    length;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(reconstruct_image != (const Image *) NULL);
  assert(reconstruct_image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if ((reconstruct_image->columns != image->columns) ||
      (reconstruct_image->rows != image->rows))
    {
      (void) ThrowMagickException(&image->exception,GetMagickModule(),
        ImageError,"ImageSizeDiffers","`%s'",image->filename);
      return((double *) NULL);
    }
  /*
    Get image distortion.
  */
  length=AllChannels+1UL;
  channel_distortion=(double *) AcquireQuantumMemory(length,
    sizeof(*channel_distortion));
  if (channel_distortion == (double *) NULL)
    ThrowFatalException(ResourceLimitFatalError,"MemoryAllocationFailed");
  (void) ResetMagickMemory(channel_distortion,0,length*
    sizeof(*channel_distortion));
  switch (metric)
  {
    case AbsoluteErrorMetric:
    {
      status=GetAbsoluteError(image,reconstruct_image,AllChannels,
        channel_distortion,exception);
      break;
    }
    case MeanAbsoluteErrorMetric:
    {
      status=GetMeanAbsoluteError(image,reconstruct_image,AllChannels,
        channel_distortion,exception);
      break;
    }
    case MeanErrorPerPixelMetric:
    {
      status=GetMeanErrorPerPixel(image,reconstruct_image,AllChannels,
        channel_distortion,exception);
      break;
    }
    case MeanSquaredErrorMetric:
    {
      status=GetMeanSquaredError(image,reconstruct_image,AllChannels,
        channel_distortion,exception);
      break;
    }
    case PeakAbsoluteErrorMetric:
    default:
    {
      status=GetPeakAbsoluteError(image,reconstruct_image,AllChannels,
        channel_distortion,exception);
      break;
    }
    case PeakSignalToNoiseRatioMetric:
    {
      status=GetPeakSignalToNoiseRatio(image,reconstruct_image,AllChannels,
        channel_distortion,exception);
      break;
    }
    case RootMeanSquaredErrorMetric:
    {
      status=GetRootMeanSquaredError(image,reconstruct_image,AllChannels,
        channel_distortion,exception);
      break;
    }
  }
  return(channel_distortion);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  I s I m a g e s E q u a l                                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsImagesEqual() measures the difference between colors at each pixel
%  location of two images.  A value other than 0 means the colors match
%  exactly.  Otherwise an error measure is computed by summing over all
%  pixels in an image the distance squared in RGB space between each image
%  pixel and its corresponding pixel in the reconstruct image.  The error
%  measure is assigned to these image members:
%
%    o mean_error_per_pixel:  The mean error for any single pixel in
%      the image.
%
%    o normalized_mean_error:  The normalized mean quantization error for
%      any single pixel in the image.  This distance measure is normalized to
%      a range between 0 and 1.  It is independent of the range of red, green,
%      and blue values in the image.
%
%    o normalized_maximum_error:  The normalized maximum quantization
%      error for any single pixel in the image.  This distance measure is
%      normalized to a range between 0 and 1.  It is independent of the range
%      of red, green, and blue values in your image.
%
%  A small normalized mean square error, accessed as
%  image->normalized_mean_error, suggests the images are very similar in
%  spatial layout and color.
%
%  The format of the IsImagesEqual method is:
%
%      MagickBooleanType IsImagesEqual(Image *image,
%        const Image *reconstruct_image)
%
%  A description of each parameter follows.
%
%    o image: the image.
%
%    o reconstruct_image: the reconstruct image.
%
*/
MagickExport MagickBooleanType IsImagesEqual(Image *image,
  const Image *reconstruct_image)
{
  long
    y;

  MagickBooleanType
    status;

  MagickRealType
    area,
    maximum_error,
    mean_error,
    mean_error_per_pixel;

  ViewInfo
    *image_view,
    *reconstruct_view;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  assert(reconstruct_image != (const Image *) NULL);
  assert(reconstruct_image->signature == MagickSignature);
  if ((reconstruct_image->columns != image->columns) ||
      (reconstruct_image->rows != image->rows))
    ThrowBinaryException(ImageError,"ImageSizeDiffers",image->filename);
  area=0.0;
  maximum_error=0.0;
  mean_error_per_pixel=0.0;
  mean_error=0.0;
  image_view=AcquireCacheView(image);
  reconstruct_view=AcquireCacheView(reconstruct_image);
  for (y=0; y < (long) image->rows; y++)
  {
    register const IndexPacket
      *indexes,
      *reconstruct_indexes;

    register const PixelPacket
      *p,
      *q;

    register long
      x;

    p=GetCacheViewVirtualPixels(image_view,0,y,image->columns,1,&image->exception);
    q=GetCacheViewVirtualPixels(reconstruct_view,0,y,reconstruct_image->columns,1,
      &image->exception);
    if ((p == (const PixelPacket *) NULL) || (q == (const PixelPacket *) NULL))
      break;
    indexes=GetCacheViewVirtualIndexQueue(image_view);
    reconstruct_indexes=GetCacheViewVirtualIndexQueue(reconstruct_view);
    for (x=0; x < (long) image->columns; x++)
    {
      MagickRealType
        distance;

      distance=fabs(p->red-(double) q->red);
      mean_error_per_pixel+=distance;
      mean_error+=distance*distance;
      if (distance > maximum_error)
        maximum_error=distance;
      area++;
      distance=fabs(p->green-(double) q->green);
      mean_error_per_pixel+=distance;
      mean_error+=distance*distance;
      if (distance > maximum_error)
        maximum_error=distance;
      area++;
      distance=fabs(p->blue-(double) q->blue);
      mean_error_per_pixel+=distance;
      mean_error+=distance*distance;
      if (distance > maximum_error)
        maximum_error=distance;
      area++;
      if (image->matte != MagickFalse)
        {
          distance=fabs(p->opacity-(double) q->opacity);
          mean_error_per_pixel+=distance;
          mean_error+=distance*distance;
          if (distance > maximum_error)
            maximum_error=distance;
          area++;
        }
      if ((image->colorspace == CMYKColorspace) &&
          (reconstruct_image->colorspace == CMYKColorspace))
        {
          distance=fabs(indexes[x]-(double) reconstruct_indexes[x]);
          mean_error_per_pixel+=distance;
          mean_error+=distance*distance;
          if (distance > maximum_error)
            maximum_error=distance;
          area++;
        }
      p++;
      q++;
    }
  }
  reconstruct_view=DestroyCacheView(reconstruct_view);
  image_view=DestroyCacheView(image_view);
  image->error.mean_error_per_pixel=(double) (mean_error_per_pixel/area);
  image->error.normalized_mean_error=(double) (QuantumScale*QuantumScale*
    mean_error/area);
  image->error.normalized_maximum_error=(double) (QuantumScale*maximum_error);
  status=image->error.mean_error_per_pixel == 0.0 ? MagickTrue : MagickFalse;
  return(status);
}
