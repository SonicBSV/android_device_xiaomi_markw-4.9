#
# vendor.prop for markw
#

# ART
PRODUCT_PROPERTY_OVERRIDES += \
dalvik.vm.dex2oat-filter=speed \
dalvik.vm.image-dex2oat-filter=speed \
dalvik.vm.dex2oat64.enabled=true \
dalvik.vm.systemuicompilerfilter=speed

# Audio
PRODUCT_PROPERTY_OVERRIDES += \
    af.fast_track_multiplier=1 \
    audio.deep_buffer.media=true \
    audio.chk.cal.us=0 \
    audio.offload.disable=false \
    audio.offload.min.duration.secs=30 \
    audio.offload.video=true \
    persist.vendor.audio.fluence.speaker=true \
    persist.vendor.audio.fluence.voicecall=true \
    persist.vendor.audio.fluence.voicerec=false \
    persist.vendor.audio.hw.binder.size_kbyte=1024 \
    persist.vendor.audio.speaker.prot.enable=false \
    ro.vendor.audio.sdk.fluencetype=fluence \
    ro.vendor.audio.sdk.ssr=false \
    ro.af.client_heap_size_kbyte=7168 \
    vendor.audio.hw.aac.encoder=true \
    vendor.audio.flac.sw.decoder.24bit=true \
    vendor.audio.read.wsatz.type=true \
    vendor.audio.offload.buffer.size.kb=64 \
    vendor.audio.offload.gapless.enabled=true \
    vendor.audio.offload.multiaac.enable=true \
    vendor.audio.offload.multiple.enabled=false \
    vendor.audio.offload.passthrough=false \
    vendor.audio.offload.track.enable=false \
    vendor.audio.dolby.ds2.enabled=false \
    vendor.audio.dolby.ds2.hardbypass=false \
    vendor.audio.parser.ip.buffer.size=262144 \
    vendor.audio.playback.mch.downsample=true \
    vendor.audio.pp.asphere.enabled=false \
    vendor.audio.safx.pbe.enabled=true \
    vendor.audio.tunnel.encode=false \
    vendor.audio.use.sw.alac.decoder=true \
    vendor.audio.use.sw.ape.decoder=true \
    vendor.audio_hal.period_size=240 \
    vendor.voice.conc.fallbackpath=deep-buffer \
    vendor.voice.path.for.pcm.voip=true \
    vendor.voice.playback.conc.disabled=true \
    vendor.voice.record.conc.disabled=false \
    vendor.voice.voip.conc.disabled=true

# Bluetooth
PRODUCT_PROPERTY_OVERRIDES += \
    vendor.qcom.bluetooth.soc=pronto \
    persist.vendor.qcom.bluetooth.soc=pronto \
    persist.vendor.qcom.bluetooth.enable.splita2dp=false \
    persist.vendor.bluetooth.modem_nv_support=true \
    ro.vendor.bluetooth.wipower=false

# Camera
PRODUCT_PROPERTY_OVERRIDES += \
vidc.enc.dcvs.extra-buff-count=2 \
media.camera.ts.monotonic=1 \
persist.vendor.camera.display.lmax=1280x720 \
persist.vendor.camera.display.umax=1920x1080 \
vendor.camera.lowpower.record.enable=1 \
persist.vendor.qti.telephony.vt_cam_interface=2 \
persist.vendor.camera.eis.enable=1 \
persist.vendor.camera.gyro.disable=0 \
persist.vendor.camera.isp.clock.optmz=0 \
persist.vendor.camera.stats.test=5 \
persist.vendor.camera.CDS=off \
persist.vendor.camera.perflock.enable=0 \
persist.vendor.camera.HAL3.enabled=1

# Cne
PRODUCT_PROPERTY_OVERRIDES += \
persist.vendor.cne.feature=1
persist.vendor.mwqem.enable=1

# Display
PRODUCT_PROPERTY_OVERRIDES += \
ro.hardware.vulkan=adreno \
ro.hardware.egl=adreno \
ro.opengles.version=196610 \
ro.vendor.display.cabl=2 \
vendor.display.disable_rotator_downscale=1 \
vendor.display.disable_skip_validate=1 \
vendor.gralloc.enable_fb_ubwc=1

# DPM
PRODUCT_PROPERTY_OVERRIDES += \
persist.vendor.dpm.feature=11 \
persist.vendor.dpm.loglevel=0 \
persist.vendor.dpm.nsrm.bkg.evt=3955 \
persist.vendor.dpmhalservice.enable=1 \
persist.vendor.mwqem.enable=1

# DRM
PRODUCT_PROPERTY_OVERRIDES += \
drm.service.enabled=true

# Fm
PRODUCT_PROPERTY_OVERRIDES += \
ro.fm.transmitter=false \
vendor.hw.fm.init=0

# Frp
PRODUCT_PROPERTY_OVERRIDES += \
ro.frp.pst=/dev/block/bootdevice/by-name/config

# Media
PRODUCT_PROPERTY_OVERRIDES += \
av.debug.disable.pers.cache=1 \
debug.stagefright.omx_default_rank=0 \
media.msm8956hw=0 \
media.stagefright.audio.sink=280 \
media.stagefright.thumbnail.prefer_hw_codecs=true \
mm.enable.smoothstreaming=true \
mm.enable.sec.smoothstreaming=true \
mmp.enable.3g2=true \
vendor.audio.hw.aac.encoder=true \
vendor.vidc.debug.level=0 \
vendor.vidc.dec.downscalar_height=1088 \
vendor.vidc.dec.downscalar_width=1920 \
vendor.vidc.disable.split.mode=1 \
vendor.vidc.enc.disable.pq=true \
vendor.vidc.enc.disable_bframes=1 \
vendor.video.disable.ubwc=1

# Perf
PRODUCT_PROPERTY_OVERRIDES += \
ro.vendor.extension_library=libqti-perfd-client.so \
ro.vendor.qti.sys.fw.bservice_enable=true

# Radio
PRODUCT_PROPERTY_OVERRIDES += \
persist.radio.multisim.config=dsds \
persist.vendor.data.iwlan.enable=true \
persist.vendor.radio.add_power_save=1 \
persist.vendor.radio.apm_sim_not_pwdn=1 \
persist.vendor.radio.custom_ecc=1 \
persist.vendor.radio.enableadvancedscan=false \
persist.vendor.radio.hw_mbn_update=0 \
persist.vendor.radio.mt_sms_ack=30 \
persist.vendor.radio.procedure_bytes=SKIP \
persist.vendor.radio.rat_on=combine \
persist.vendor.radio.sib16_support=1

# Recovery
PRODUCT_PROPERTY_OVERRIDES += \
persist.vendor.recovery_update=false

# Time Services
PRODUCT_PROPERTY_OVERRIDES += \
persist.backup.ntpServer=0.pool.ntp.org

# Tcp
PRODUCT_PROPERTY_OVERRIDES += \
net.tcp.2g_init_rwnd=10

# Usb
PRODUCT_PROPERTY_OVERRIDES += \
persist.vendor.usb.config.extra=none

# Wifi
PRODUCT_PROPERTY_OVERRIDES += \
wifi.interface=wlan0

# Wifi-Display
PRODUCT_PROPERTY_OVERRIDES += \
persist.sys.wfd.virtual=0
