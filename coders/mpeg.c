/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                        M   M  PPPP   EEEEE   GGGG                           %
%                        MM MM  P   P  E      G                               %
%                        M M M  PPPP   EEE    G  GG                           %
%                        M   M  P      E      G   G                           %
%                        M   M  P      EEEEE   GGGG                           %
%                                                                             %
%                                                                             %
%                       Read/Write MPEG Image Format                          %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                 July 1999                                   %
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
*/
/*
  Include declarations.
*/
#include "magick/studio.h"
#include "magick/blob.h"
#include "magick/blob-private.h"
#include "magick/constitute.h"
#include "magick/delegate.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/geometry.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/layer.h"
#include "magick/list.h"
#include "magick/log.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/resource_.h"
#include "magick/quantum-private.h"
#include "magick/static.h"
#include "magick/string_.h"
#include "magick/module.h"
#include "magick/transform.h"
#include "magick/utility.h"

/*
  Forward declarations.
*/
static MagickBooleanType
  WriteMPEGImage(const ImageInfo *image_info,Image *image);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s M P E G                                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsMPEG() returns MagickTrue if the image format type, identified by the
%  magick string, is MPEG.
%
%  The format of the IsMPEG method is:
%
%      MagickBooleanType IsMPEG(const unsigned char *magick,const size_t length)
%
%  A description of each parameter follows:
%
%    o magick: This string is generally the first few bytes of an image file
%      or blob.
%
%    o length: Specifies the length of the magick string.
%
*/
static MagickBooleanType IsMPEG(const unsigned char *magick,const size_t length)
{
  if (length < 4)
    return(MagickFalse);
  if (memcmp(magick,"\000\000\001\263",4) == 0)
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d M P E G I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadMPEGImage() reads an binary file in the MPEG video stream format
%  and returns it.  It allocates the memory necessary for the new Image
%  structure and returns a pointer to the new image.
%
%  The format of the ReadMPEGImage method is:
%
%      Image *ReadMPEGImage(const ImageInfo *image_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static Image *ReadMPEGImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
#define ReadMPEGIntermediateFormat "pam"

  Image
    *image,
    *images;

  ImageInfo
    *read_info;

  MagickBooleanType
    status;

  register long
    i;

  /*
    Open image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  image=AcquireImage(image_info);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  (void) CloseBlob(image);
  (void) DestroyImageList(image);
  /*
    Convert MPEG to PPM with delegate.
  */
  image=AcquireImage(image_info);
  read_info=CloneImageInfo(image_info);
  (void) InvokeDelegate(read_info,image,"mpeg:decode",(char *) NULL,exception);
  image=DestroyImage(image);
  /*
    Read PPM files.
  */
  images=NewImageList();
  for (i=(long) read_info->scene; ; i++)
  {
    (void) FormatMagickString(read_info->filename,MaxTextExtent,"%s%ld.%s",
      read_info->unique,i,ReadMPEGIntermediateFormat);
    if (IsPathAccessible(read_info->filename) == MagickFalse)
      break;
    image=ReadImage(read_info,exception);
    if (image == (Image *) NULL)
      break;
    (void) CopyMagickString(image->magick,image_info->magick,MaxTextExtent);
    image->scene=(unsigned long) i;
    AppendImageToList(&images,image);
    if (read_info->number_scenes != 0)
      if (i >= (long) (read_info->scene+read_info->number_scenes-1))
        break;
  }
  /*
    Relinquish resources.
  */
  for (i=0; ; i++)
  {
    (void) FormatMagickString(read_info->filename,MaxTextExtent,"%s%ld.%s",
      read_info->unique,i,ReadMPEGIntermediateFormat);
    if (IsPathAccessible(read_info->filename) == MagickFalse)
      break;
    (void) RelinquishUniqueFileResource(read_info->filename);
  }
  read_info=DestroyImageInfo(read_info);
  return(images);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r M P E G I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterMPEGImage() adds attributes for the MPEG image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterMPEGImage method is:
%
%      unsigned long RegisterMPEGImage(void)
%
*/
ModuleExport unsigned long RegisterMPEGImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("MOV");
  entry->decoder=(DecodeImageHandler *) ReadMPEGImage;
  entry->encoder=(EncodeImageHandler *) WriteMPEGImage;
  entry->magick=(IsImageFormatHandler *) IsMPEG;
  entry->blob_support=MagickFalse;
  entry->description=ConstantString("MPEG Video Stream");
  entry->module=ConstantString("MPEG");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("MPEG");
  entry->decoder=(DecodeImageHandler *) ReadMPEGImage;
  entry->encoder=(EncodeImageHandler *) WriteMPEGImage;
  entry->magick=(IsImageFormatHandler *) IsMPEG;
  entry->blob_support=MagickFalse;
  entry->description=ConstantString("MPEG Video Stream");
  entry->module=ConstantString("MPEG");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("MPG");
  entry->decoder=(DecodeImageHandler *) ReadMPEGImage;
  entry->encoder=(EncodeImageHandler *) WriteMPEGImage;
  entry->magick=(IsImageFormatHandler *) IsMPEG;
  entry->blob_support=MagickFalse;
  entry->description=ConstantString("MPEG Video Stream");
  entry->module=ConstantString("MPEG");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("MP4");
  entry->decoder=(DecodeImageHandler *) ReadMPEGImage;
  entry->encoder=(EncodeImageHandler *) WriteMPEGImage;
  entry->magick=(IsImageFormatHandler *) IsMPEG;
  entry->blob_support=MagickFalse;
  entry->description=ConstantString("MPEG-4 Video Stream");
  entry->module=ConstantString("MPEG");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("M2V");
  entry->decoder=(DecodeImageHandler *) ReadMPEGImage;
  entry->encoder=(EncodeImageHandler *) WriteMPEGImage;
  entry->magick=(IsImageFormatHandler *) IsMPEG;
  entry->blob_support=MagickFalse;
  entry->description=ConstantString("MPEG Video Stream");
  entry->module=ConstantString("MPEG");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("M4V");
  entry->decoder=(DecodeImageHandler *) ReadMPEGImage;
  entry->encoder=(EncodeImageHandler *) WriteMPEGImage;
  entry->magick=(IsImageFormatHandler *) IsMPEG;
  entry->blob_support=MagickFalse;
  entry->description=ConstantString("Raw MPEG-4 Video");
  entry->module=ConstantString("MPEG");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("WMV");
  entry->decoder=(DecodeImageHandler *) ReadMPEGImage;
  entry->encoder=(EncodeImageHandler *) WriteMPEGImage;
  entry->magick=(IsImageFormatHandler *) IsMPEG;
  entry->blob_support=MagickFalse;
  entry->description=ConstantString("Windows Media Video");
  entry->module=ConstantString("MPEG");
  (void) RegisterMagickInfo(entry);
  return(MagickImageCoderSignature);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r M P E G I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterMPEGImage() removes format registrations made by the
%  BIM module from the list of supported formats.
%
%  The format of the UnregisterBIMImage method is:
%
%      UnregisterMPEGImage(void)
%
*/
ModuleExport void UnregisterMPEGImage(void)
{
  (void) UnregisterMagickInfo("WMV");
  (void) UnregisterMagickInfo("M4V");
  (void) UnregisterMagickInfo("M2V");
  (void) UnregisterMagickInfo("MP4");
  (void) UnregisterMagickInfo("MPG");
  (void) UnregisterMagickInfo("MPEG");
  (void) UnregisterMagickInfo("MOV");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e M P E G I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteMPEGImage() writes an image to a file in MPEG video stream format.
%  Lawrence Livermore National Laboratory (LLNL) contributed code to adjust
%  the MPEG parameters to correspond to the compression quality setting.
%
%  The format of the WriteMPEGImage method is:
%
%      MagickBooleanType WriteMPEGImage(const ImageInfo *image_info,
%        Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: the image info.
%
%    o image:  The image.
%
*/

static inline double MagickMax(const double x,const double y)
{
  if (x > y)
    return(x);
  return(y);
}

static inline double MagickMin(const double x,const double y)
{
  if (x < y)
    return(x);
  return(y);
}

static MagickBooleanType CopyDelegateFile(const char *source,
  const char *destination)
{
  int
    destination_file,
    source_file;

  MagickBooleanType
    status;

  register size_t
    i;

  size_t
    length,
    quantum;

  ssize_t
    count;

  struct stat
    attributes;

  unsigned char
    *buffer;

  /*
    Return if destination file already exists and is not empty.
  */
  assert(source != (const char *) NULL);
  assert(destination != (char *) NULL);
  status=GetPathAttributes(destination,&attributes);
  if ((status != MagickFalse) && (attributes.st_size != 0))
    return(MagickTrue);
  /*
    Copy source file to destination.
  */
  destination_file=open(destination,O_WRONLY | O_BINARY | O_CREAT,S_MODE);
  if (destination_file == -1)
    return(MagickFalse);
  source_file=open(source,O_RDONLY | O_BINARY);
  if (source_file == -1)
    {
      (void) close(destination_file);
      return(MagickFalse);
    }
  quantum=(size_t) MagickMaxBufferExtent;
  if ((fstat(source_file,&attributes) == 0) && (attributes.st_size != 0))
    quantum=(size_t) MagickMin((double) attributes.st_size,
      MagickMaxBufferExtent);
  buffer=(unsigned char *) AcquireQuantumMemory(quantum,sizeof(*buffer));
  if (buffer == (unsigned char *) NULL)
    {
      (void) close(source_file);
      (void) close(destination_file);
      return(MagickFalse);
    }
  length=0;
  for (i=0; ; i+=count)
  {
    count=(ssize_t) read(source_file,buffer,quantum);
    if (count <= 0)
      break;
    length=(size_t) count;
    count=(ssize_t) write(destination_file,buffer,length);
    if ((size_t) count != length)
      break;
  }
  (void) close(destination_file);
  (void) close(source_file);
  buffer=(unsigned char *) RelinquishMagickMemory(buffer);
  return(i != 0 ? MagickTrue : MagickFalse);
}

static MagickBooleanType WriteMPEGImage(const ImageInfo *image_info,
  Image *image)
{
#define WriteMPEGIntermediateFormat "jpg"

  char
    basename[MaxTextExtent],
    filename[MaxTextExtent];

  double
    delay;

  Image
    *coalesce_image;

  ImageInfo
    *write_info;

  int
    file;

  MagickBooleanType
    status;

  register Image
    *p;

  register long
    i;

  size_t
    length;

  unsigned char
    *blob;

  unsigned long
    count,
    scene;

  /*
    Open output image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  status=OpenBlob(image_info,image,WriteBinaryBlobMode,&image->exception);
  if (status == MagickFalse)
    return(status);
  (void) CloseBlob(image);
  /*
    Write JPEG files.
  */
  coalesce_image=CoalesceImages(image,&image->exception);
  if (coalesce_image == (Image *) NULL)
    return(MagickFalse);
  file=AcquireUniqueFileResource(basename);
  if (file != -1)
    file=close(file)-1;
  (void) FormatMagickString(coalesce_image->filename,MaxTextExtent,"%s",
    basename);
  count=0;
  write_info=CloneImageInfo(image_info);
  for (p=coalesce_image; p != (Image *) NULL; p=GetNextImageInList(p))
  {
    char
      previous_image[MaxTextExtent];

    blob=(unsigned char *) NULL;
    length=0;
    scene=p->scene;
    delay=100.0*p->delay/MagickMax(1.0*p->ticks_per_second,1.0);
    for (i=0; i < (long) MagickMax((1.0*delay+1.0)/3.0,1.0); i++)
    {
      p->scene=count;
      count++;
      status=MagickFalse;
      switch (i)
      {
        case 0:
        {
          Image
            *frame;

          (void) FormatMagickString(p->filename,MaxTextExtent,"%s%lu.%s",
            basename,p->scene,WriteMPEGIntermediateFormat);
          (void) FormatMagickString(filename,MaxTextExtent,"%s%lu.%s",
            basename,p->scene,WriteMPEGIntermediateFormat);
          (void) FormatMagickString(previous_image,MaxTextExtent,
            "%s%lu.%s",basename,p->scene,WriteMPEGIntermediateFormat);
          frame=CloneImage(p,0,0,MagickTrue,&p->exception);
          if (frame == (Image *) NULL)
            break;
          status=WriteImage(write_info,frame);
          frame=DestroyImage(frame);
          break;
        }
        case 1:
        {
          blob=(unsigned char *) FileToBlob(previous_image,~0UL,&length,
            &image->exception);
        }
        default:
        {
          (void) FormatMagickString(filename,MaxTextExtent,"%s%lu.%s",
            basename,p->scene,WriteMPEGIntermediateFormat);
          if (length > 0)
            status=BlobToFile(filename,blob,length,&image->exception);
          break;
        }
      }
      if (image->debug != MagickFalse)
        {
          if (status != MagickFalse)
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "%lu. Wrote JPEG file for scene %lu:",i,p->scene);
          else
            (void) LogMagickEvent(CoderEvent,GetMagickModule(),
              "%lu. Failed to write JPEG file for scene %lu:",i,p->scene);
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),"%s",
            filename);
        }
    }
    p->scene=scene;
    if (blob != (unsigned char *) NULL)
      blob=(unsigned char *) RelinquishMagickMemory(blob);
    if (status == MagickFalse)
      break;
  }
  /*
    Convert JPEG to MPEG.
  */
  (void) CopyMagickString(coalesce_image->magick_filename,basename,
    MaxTextExtent);
  (void) CopyMagickString(coalesce_image->filename,basename,MaxTextExtent);
  GetPathComponent(image_info->filename,ExtensionPath,coalesce_image->magick);
  if (*coalesce_image->magick == '\0')
    (void) CopyMagickString(coalesce_image->magick,image->magick,MaxTextExtent);
  status=InvokeDelegate(write_info,coalesce_image,(char *) NULL,"mpeg:encode",
    &image->exception);
  (void) FormatMagickString(write_info->filename,MaxTextExtent,"%s.%s",
    write_info->unique,coalesce_image->magick);
  status=CopyDelegateFile(write_info->filename,image->filename);
  (void) RelinquishUniqueFileResource(write_info->filename);
  write_info=DestroyImageInfo(write_info);
  /*
    Relinquish resources.
  */
  count=0;
  for (p=coalesce_image; p != (Image *) NULL; p=GetNextImageInList(p))
  {
    delay=100.0*p->delay/MagickMax(1.0*p->ticks_per_second,1.0);
    for (i=0; i < (long) MagickMax((1.0*delay+1.0)/3.0,1.0); i++)
    {
      (void) FormatMagickString(p->filename,MaxTextExtent,"%s%lu.%s",
        basename,count++,WriteMPEGIntermediateFormat);
      (void) RelinquishUniqueFileResource(p->filename);
    }
    (void) CopyMagickString(p->filename,image_info->filename,MaxTextExtent);
  }
  (void) RelinquishUniqueFileResource(basename);
  coalesce_image=DestroyImageList(coalesce_image);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(CoderEvent,GetMagickModule(),"exit");
  return(status);
}