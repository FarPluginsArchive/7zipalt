PRODUCT = "7-Zip Alternative"
CANDLEFLAGS = -nologo -dSourceDir=$(OUTDIR) -dPlatform=$(PLATFORM) -dProduct=$(PRODUCT) -dVersion=$(VERSION) -dMinFarVersion=$(FAR_VERSION) -d7z.dll=$(7ZDLL) -I$(INSTALLER) -dcl:high
LIGHTFLAGS = -nologo -cultures:en-us -spdb -sval -b $(INSTALLER)
WIXOBJ = $(OUTDIR)\installer.wixobj $(OUTDIR)\ui.wixobj $(OUTDIR)\plugin.wixobj

$(DISTRIB).msi: $(WIXOBJ) $(INSTALLER)\ui_en-us.wxl $(INSTALLER)\banner.jpg $(INSTALLER)\dialog.jpg $(INSTALLER)\exclam.ico $(INSTALLER)\info.ico
  light $(LIGHTFLAGS) -loc $(INSTALLER)\ui_en-us.wxl -out $@ $(WIXOBJ)

{$(INSTALLER)}.wxs{$(OUTDIR)}.wixobj::
  candle $(CANDLEFLAGS) -out $(OUTDIR)\ $<

$(INSTALLER)\installer.wxs $(INSTALLER)\plugin.wxs: $(INSTALLER)\installer.wxi $(INSTALLER)\plugin.wxi

.SUFFIXES: .wxs
