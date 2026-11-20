# ffmpeg.mk: el pedazo de FFmpeg que usa quixjs (decodificar H.264 y AAC, los
# MP4 de runtime/video) y como se compila. Lo incluyen el Makefile (PC),
# portmaster/portmaster.mk y android/CMakeLists.txt (sus listas).
#
# Las listas salen de las dependencias (.d) de una compilacion de FFmpeg con
# solo esos dos decodificadores (ver README.md): las fuentes de cada objeto.
# Iguales en Linux y Android de la misma arquitectura; cambia config/<destino>.

FFMPEG_DIR ?= runtime/thirdparty/ffmpeg

# las opciones de su config.mak (-w: es codigo de fuera, sin sus avisos)
FFMPEG_CFLAGS = -I$(FFMPEG_DIR) -I$(FFMPEG_DIR)/gen -DHAVE_AV_CONFIG_H -std=c11 \
  -D_ISOC99_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_POSIX_C_SOURCE=200112 -D_XOPEN_SOURCE=600 \
  -O3 -fomit-frame-pointer -fno-math-errno -fno-signed-zeros -pthread -w
# lo que necesita quien usa sus cabeceras publicas (runtime/video)
FFMPEG_INCLUDES = -I$(FFMPEG_DIR) -I$(FFMPEG_DIR)/gen

FFMPEG_SRCS := \
  $(FFMPEG_DIR)/libavcodec/aacdec.c \
  $(FFMPEG_DIR)/libavcodec/aacps_common.c \
  $(FFMPEG_DIR)/libavcodec/aacps_float.c \
  $(FFMPEG_DIR)/libavcodec/aacpsdsp_float.c \
  $(FFMPEG_DIR)/libavcodec/aacsbr.c \
  $(FFMPEG_DIR)/libavcodec/aactab.c \
  $(FFMPEG_DIR)/libavcodec/ac3_parser.c \
  $(FFMPEG_DIR)/libavcodec/adts_header.c \
  $(FFMPEG_DIR)/libavcodec/adts_parser.c \
  $(FFMPEG_DIR)/libavcodec/allcodecs.c \
  $(FFMPEG_DIR)/libavcodec/atsc_a53.c \
  $(FFMPEG_DIR)/libavcodec/avcodec.c \
  $(FFMPEG_DIR)/libavcodec/avdct.c \
  $(FFMPEG_DIR)/libavcodec/avfft.c \
  $(FFMPEG_DIR)/libavcodec/avpacket.c \
  $(FFMPEG_DIR)/libavcodec/bitstream.c \
  $(FFMPEG_DIR)/libavcodec/bitstream_filters.c \
  $(FFMPEG_DIR)/libavcodec/bsf.c \
  $(FFMPEG_DIR)/libavcodec/cabac.c \
  $(FFMPEG_DIR)/libavcodec/cbrt_data.c \
  $(FFMPEG_DIR)/libavcodec/codec_desc.c \
  $(FFMPEG_DIR)/libavcodec/codec_par.c \
  $(FFMPEG_DIR)/libavcodec/d3d11va.c \
  $(FFMPEG_DIR)/libavcodec/decode.c \
  $(FFMPEG_DIR)/libavcodec/dirac.c \
  $(FFMPEG_DIR)/libavcodec/dv_profile.c \
  $(FFMPEG_DIR)/libavcodec/encode.c \
  $(FFMPEG_DIR)/libavcodec/error_resilience.c \
  $(FFMPEG_DIR)/libavcodec/faandct.c \
  $(FFMPEG_DIR)/libavcodec/faanidct.c \
  $(FFMPEG_DIR)/libavcodec/fdctdsp.c \
  $(FFMPEG_DIR)/libavcodec/get_buffer.c \
  $(FFMPEG_DIR)/libavcodec/golomb.c \
  $(FFMPEG_DIR)/libavcodec/h2645_parse.c \
  $(FFMPEG_DIR)/libavcodec/h2645_sei.c \
  $(FFMPEG_DIR)/libavcodec/h2645_vui.c \
  $(FFMPEG_DIR)/libavcodec/h2645data.c \
  $(FFMPEG_DIR)/libavcodec/h264_cabac.c \
  $(FFMPEG_DIR)/libavcodec/h264_cavlc.c \
  $(FFMPEG_DIR)/libavcodec/h264_direct.c \
  $(FFMPEG_DIR)/libavcodec/h264_loopfilter.c \
  $(FFMPEG_DIR)/libavcodec/h264_mb.c \
  $(FFMPEG_DIR)/libavcodec/h264_parse.c \
  $(FFMPEG_DIR)/libavcodec/h264_picture.c \
  $(FFMPEG_DIR)/libavcodec/h264_ps.c \
  $(FFMPEG_DIR)/libavcodec/h264_refs.c \
  $(FFMPEG_DIR)/libavcodec/h264_sei.c \
  $(FFMPEG_DIR)/libavcodec/h264_slice.c \
  $(FFMPEG_DIR)/libavcodec/h264chroma.c \
  $(FFMPEG_DIR)/libavcodec/h264data.c \
  $(FFMPEG_DIR)/libavcodec/h264dec.c \
  $(FFMPEG_DIR)/libavcodec/h264dsp.c \
  $(FFMPEG_DIR)/libavcodec/h264idct.c \
  $(FFMPEG_DIR)/libavcodec/h264pred.c \
  $(FFMPEG_DIR)/libavcodec/h264qpel.c \
  $(FFMPEG_DIR)/libavcodec/h274.c \
  $(FFMPEG_DIR)/libavcodec/idctdsp.c \
  $(FFMPEG_DIR)/libavcodec/imgconvert.c \
  $(FFMPEG_DIR)/libavcodec/jfdctfst.c \
  $(FFMPEG_DIR)/libavcodec/jfdctint.c \
  $(FFMPEG_DIR)/libavcodec/jni.c \
  $(FFMPEG_DIR)/libavcodec/jrevdct.c \
  $(FFMPEG_DIR)/libavcodec/kbdwin.c \
  $(FFMPEG_DIR)/libavcodec/mathtables.c \
  $(FFMPEG_DIR)/libavcodec/me_cmp.c \
  $(FFMPEG_DIR)/libavcodec/mediacodec.c \
  $(FFMPEG_DIR)/libavcodec/mpeg12framerate.c \
  $(FFMPEG_DIR)/libavcodec/mpeg4audio.c \
  $(FFMPEG_DIR)/libavcodec/mpeg4audio_sample_rates.c \
  $(FFMPEG_DIR)/libavcodec/options.c \
  $(FFMPEG_DIR)/libavcodec/parser.c \
  $(FFMPEG_DIR)/libavcodec/parsers.c \
  $(FFMPEG_DIR)/libavcodec/profiles.c \
  $(FFMPEG_DIR)/libavcodec/pthread.c \
  $(FFMPEG_DIR)/libavcodec/pthread_frame.c \
  $(FFMPEG_DIR)/libavcodec/pthread_slice.c \
  $(FFMPEG_DIR)/libavcodec/qsv_api.c \
  $(FFMPEG_DIR)/libavcodec/raw.c \
  $(FFMPEG_DIR)/libavcodec/refstruct.c \
  $(FFMPEG_DIR)/libavcodec/sbrdsp.c \
  $(FFMPEG_DIR)/libavcodec/simple_idct.c \
  $(FFMPEG_DIR)/libavcodec/sinewin.c \
  $(FFMPEG_DIR)/libavcodec/startcode.c \
  $(FFMPEG_DIR)/libavcodec/utils.c \
  $(FFMPEG_DIR)/libavcodec/version.c \
  $(FFMPEG_DIR)/libavcodec/videodsp.c \
  $(FFMPEG_DIR)/libavcodec/vlc.c \
  $(FFMPEG_DIR)/libavcodec/vorbis_parser.c \
  $(FFMPEG_DIR)/libavcodec/xiph.c \
  $(FFMPEG_DIR)/libavutil/adler32.c \
  $(FFMPEG_DIR)/libavutil/aes.c \
  $(FFMPEG_DIR)/libavutil/aes_ctr.c \
  $(FFMPEG_DIR)/libavutil/ambient_viewing_environment.c \
  $(FFMPEG_DIR)/libavutil/audio_fifo.c \
  $(FFMPEG_DIR)/libavutil/avsscanf.c \
  $(FFMPEG_DIR)/libavutil/avstring.c \
  $(FFMPEG_DIR)/libavutil/base64.c \
  $(FFMPEG_DIR)/libavutil/blowfish.c \
  $(FFMPEG_DIR)/libavutil/bprint.c \
  $(FFMPEG_DIR)/libavutil/buffer.c \
  $(FFMPEG_DIR)/libavutil/camellia.c \
  $(FFMPEG_DIR)/libavutil/cast5.c \
  $(FFMPEG_DIR)/libavutil/channel_layout.c \
  $(FFMPEG_DIR)/libavutil/cpu.c \
  $(FFMPEG_DIR)/libavutil/crc.c \
  $(FFMPEG_DIR)/libavutil/csp.c \
  $(FFMPEG_DIR)/libavutil/des.c \
  $(FFMPEG_DIR)/libavutil/detection_bbox.c \
  $(FFMPEG_DIR)/libavutil/dict.c \
  $(FFMPEG_DIR)/libavutil/display.c \
  $(FFMPEG_DIR)/libavutil/dovi_meta.c \
  $(FFMPEG_DIR)/libavutil/downmix_info.c \
  $(FFMPEG_DIR)/libavutil/encryption_info.c \
  $(FFMPEG_DIR)/libavutil/error.c \
  $(FFMPEG_DIR)/libavutil/eval.c \
  $(FFMPEG_DIR)/libavutil/executor.c \
  $(FFMPEG_DIR)/libavutil/fifo.c \
  $(FFMPEG_DIR)/libavutil/file.c \
  $(FFMPEG_DIR)/libavutil/file_open.c \
  $(FFMPEG_DIR)/libavutil/film_grain_params.c \
  $(FFMPEG_DIR)/libavutil/fixed_dsp.c \
  $(FFMPEG_DIR)/libavutil/float_dsp.c \
  $(FFMPEG_DIR)/libavutil/frame.c \
  $(FFMPEG_DIR)/libavutil/hash.c \
  $(FFMPEG_DIR)/libavutil/hdr_dynamic_metadata.c \
  $(FFMPEG_DIR)/libavutil/hdr_dynamic_vivid_metadata.c \
  $(FFMPEG_DIR)/libavutil/hmac.c \
  $(FFMPEG_DIR)/libavutil/hwcontext.c \
  $(FFMPEG_DIR)/libavutil/hwcontext_stub.c \
  $(FFMPEG_DIR)/libavutil/imgutils.c \
  $(FFMPEG_DIR)/libavutil/integer.c \
  $(FFMPEG_DIR)/libavutil/intmath.c \
  $(FFMPEG_DIR)/libavutil/lfg.c \
  $(FFMPEG_DIR)/libavutil/lls.c \
  $(FFMPEG_DIR)/libavutil/log.c \
  $(FFMPEG_DIR)/libavutil/log2_tab.c \
  $(FFMPEG_DIR)/libavutil/lzo.c \
  $(FFMPEG_DIR)/libavutil/mastering_display_metadata.c \
  $(FFMPEG_DIR)/libavutil/mathematics.c \
  $(FFMPEG_DIR)/libavutil/md5.c \
  $(FFMPEG_DIR)/libavutil/mem.c \
  $(FFMPEG_DIR)/libavutil/murmur3.c \
  $(FFMPEG_DIR)/libavutil/opt.c \
  $(FFMPEG_DIR)/libavutil/parseutils.c \
  $(FFMPEG_DIR)/libavutil/pixdesc.c \
  $(FFMPEG_DIR)/libavutil/pixelutils.c \
  $(FFMPEG_DIR)/libavutil/random_seed.c \
  $(FFMPEG_DIR)/libavutil/rational.c \
  $(FFMPEG_DIR)/libavutil/rc4.c \
  $(FFMPEG_DIR)/libavutil/reverse.c \
  $(FFMPEG_DIR)/libavutil/ripemd.c \
  $(FFMPEG_DIR)/libavutil/samplefmt.c \
  $(FFMPEG_DIR)/libavutil/sha.c \
  $(FFMPEG_DIR)/libavutil/sha512.c \
  $(FFMPEG_DIR)/libavutil/slicethread.c \
  $(FFMPEG_DIR)/libavutil/spherical.c \
  $(FFMPEG_DIR)/libavutil/stereo3d.c \
  $(FFMPEG_DIR)/libavutil/tea.c \
  $(FFMPEG_DIR)/libavutil/threadmessage.c \
  $(FFMPEG_DIR)/libavutil/time.c \
  $(FFMPEG_DIR)/libavutil/timecode.c \
  $(FFMPEG_DIR)/libavutil/tree.c \
  $(FFMPEG_DIR)/libavutil/twofish.c \
  $(FFMPEG_DIR)/libavutil/tx.c \
  $(FFMPEG_DIR)/libavutil/tx_double.c \
  $(FFMPEG_DIR)/libavutil/tx_float.c \
  $(FFMPEG_DIR)/libavutil/tx_int32.c \
  $(FFMPEG_DIR)/libavutil/utils.c \
  $(FFMPEG_DIR)/libavutil/uuid.c \
  $(FFMPEG_DIR)/libavutil/version.c \
  $(FFMPEG_DIR)/libavutil/video_enc_params.c \
  $(FFMPEG_DIR)/libavutil/video_hint.c \
  $(FFMPEG_DIR)/libavutil/xga_font_data.c \
  $(FFMPEG_DIR)/libavutil/xtea.c

# x86_64 (PC, Android x86_64): sin ensamblador (nasm), solo lo de C
FFMPEG_SRCS_x86_64 := \
  $(FFMPEG_DIR)/libavcodec/x86/aacpsdsp_init.c \
  $(FFMPEG_DIR)/libavcodec/x86/constants.c \
  $(FFMPEG_DIR)/libavcodec/x86/fdct.c \
  $(FFMPEG_DIR)/libavcodec/x86/fdctdsp_init.c \
  $(FFMPEG_DIR)/libavcodec/x86/h264_intrapred_init.c \
  $(FFMPEG_DIR)/libavcodec/x86/h264_qpel.c \
  $(FFMPEG_DIR)/libavcodec/x86/h264chroma_init.c \
  $(FFMPEG_DIR)/libavcodec/x86/h264dsp_init.c \
  $(FFMPEG_DIR)/libavcodec/x86/idctdsp_init.c \
  $(FFMPEG_DIR)/libavcodec/x86/me_cmp_init.c \
  $(FFMPEG_DIR)/libavcodec/x86/sbrdsp_init.c \
  $(FFMPEG_DIR)/libavcodec/x86/videodsp_init.c \
  $(FFMPEG_DIR)/libavutil/x86/cpu.c \
  $(FFMPEG_DIR)/libavutil/x86/fixed_dsp_init.c \
  $(FFMPEG_DIR)/libavutil/x86/float_dsp_init.c \
  $(FFMPEG_DIR)/libavutil/x86/imgutils_init.c \
  $(FFMPEG_DIR)/libavutil/x86/lls_init.c

# aarch64 (PortMaster, Android arm64): con NEON
FFMPEG_SRCS_aarch64 := \
  $(FFMPEG_DIR)/libavcodec/aarch64/aacpsdsp_init_aarch64.c \
  $(FFMPEG_DIR)/libavcodec/aarch64/aacpsdsp_neon.S \
  $(FFMPEG_DIR)/libavcodec/aarch64/h264chroma_init_aarch64.c \
  $(FFMPEG_DIR)/libavcodec/aarch64/h264cmc_neon.S \
  $(FFMPEG_DIR)/libavcodec/aarch64/h264dsp_init_aarch64.c \
  $(FFMPEG_DIR)/libavcodec/aarch64/h264dsp_neon.S \
  $(FFMPEG_DIR)/libavcodec/aarch64/h264idct_neon.S \
  $(FFMPEG_DIR)/libavcodec/aarch64/h264pred_init.c \
  $(FFMPEG_DIR)/libavcodec/aarch64/h264pred_neon.S \
  $(FFMPEG_DIR)/libavcodec/aarch64/h264qpel_init_aarch64.c \
  $(FFMPEG_DIR)/libavcodec/aarch64/h264qpel_neon.S \
  $(FFMPEG_DIR)/libavcodec/aarch64/hpeldsp_neon.S \
  $(FFMPEG_DIR)/libavcodec/aarch64/idctdsp_init_aarch64.c \
  $(FFMPEG_DIR)/libavcodec/aarch64/idctdsp_neon.S \
  $(FFMPEG_DIR)/libavcodec/aarch64/me_cmp_init_aarch64.c \
  $(FFMPEG_DIR)/libavcodec/aarch64/me_cmp_neon.S \
  $(FFMPEG_DIR)/libavcodec/aarch64/sbrdsp_init_aarch64.c \
  $(FFMPEG_DIR)/libavcodec/aarch64/sbrdsp_neon.S \
  $(FFMPEG_DIR)/libavcodec/aarch64/simple_idct_neon.S \
  $(FFMPEG_DIR)/libavcodec/aarch64/videodsp.S \
  $(FFMPEG_DIR)/libavcodec/aarch64/videodsp_init.c \
  $(FFMPEG_DIR)/libavutil/aarch64/cpu.c \
  $(FFMPEG_DIR)/libavutil/aarch64/float_dsp_init.c \
  $(FFMPEG_DIR)/libavutil/aarch64/float_dsp_neon.S \
  $(FFMPEG_DIR)/libavutil/aarch64/tx_float_init.c \
  $(FFMPEG_DIR)/libavutil/aarch64/tx_float_neon.S
