MD cci
3dstool -xvt01234567f cci cci\0.cxi cci\1.cxi cci\2.cxi cci\3.cxi cci\4.cxi cci\5.cxi cci\6.cxi cci\7.cxi %1 --header cci\ncsdheader.bin
3dstool -evtf cxi cci\0.cxi --exhxor CTR-P-CATP0.exh.xorpad --exefsxor CTR-P-CTAP0.exefs.xorpad --romfsxor CTR-P-CTAP0.romfs.xorpad
3dstool -evtf cxi cci\1.cxi --exhxor CTR-P-CATP1.exh.xorpad --exefsxor CTR-P-CTAP1.exefs.xorpad --romfsxor CTR-P-CTAP1.romfs.xorpad
3dstool -evtf cxi cci\2.cxi --exhxor CTR-P-CATP2.exh.xorpad --exefsxor CTR-P-CTAP2.exefs.xorpad --romfsxor CTR-P-CTAP2.romfs.xorpad
3dstool -evtf cxi cci\3.cxi --exhxor CTR-P-CATP3.exh.xorpad --exefsxor CTR-P-CTAP3.exefs.xorpad --romfsxor CTR-P-CTAP3.romfs.xorpad
3dstool -evtf cxi cci\4.cxi --exhxor CTR-P-CATP4.exh.xorpad --exefsxor CTR-P-CTAP4.exefs.xorpad --romfsxor CTR-P-CTAP4.romfs.xorpad
3dstool -evtf cxi cci\5.cxi --exhxor CTR-P-CATP5.exh.xorpad --exefsxor CTR-P-CTAP5.exefs.xorpad --romfsxor CTR-P-CTAP5.romfs.xorpad
3dstool -evtf cxi cci\6.cxi --exhxor CTR-P-CATP6.exh.xorpad --exefsxor CTR-P-CTAP6.exefs.xorpad --romfsxor CTR-P-CTAP6.romfs.xorpad
3dstool -evtf cxi cci\7.cxi --exhxor CTR-P-CATP7.exh.xorpad --exefsxor CTR-P-CTAP7.exefs.xorpad --romfsxor CTR-P-CTAP7.romfs.xorpad
MD cci\cxi0
3dstool -xvtf cxi cci\0.cxi --header cci\cxi0\ncchheader.bin --exh cci\cxi0\exh.bin --ace cci\cxi0\ace.bin --plain cci\cxi0\plain.bin --exefs cci\cxi0\exefs.bin --romfs cci\cxi0\romfs.bin
MD cci\cxi1
3dstool -xvtf cxi cci\1.cxi --header cci\cxi1\ncchheader.bin --exh cci\cxi1\exh.bin --ace cci\cxi1\ace.bin --plain cci\cxi1\plain.bin --exefs cci\cxi1\exefs.bin --romfs cci\cxi1\romfs.bin
MD cci\cxi2
3dstool -xvtf cxi cci\2.cxi --header cci\cxi2\ncchheader.bin --exh cci\cxi2\exh.bin --ace cci\cxi2\ace.bin --plain cci\cxi2\plain.bin --exefs cci\cxi2\exefs.bin --romfs cci\cxi2\romfs.bin
MD cci\cxi3
3dstool -xvtf cxi cci\3.cxi --header cci\cxi3\ncchheader.bin --exh cci\cxi3\exh.bin --ace cci\cxi3\ace.bin --plain cci\cxi3\plain.bin --exefs cci\cxi3\exefs.bin --romfs cci\cxi3\romfs.bin
MD cci\cxi4
3dstool -xvtf cxi cci\4.cxi --header cci\cxi4\ncchheader.bin --exh cci\cxi4\exh.bin --ace cci\cxi4\ace.bin --plain cci\cxi4\plain.bin --exefs cci\cxi4\exefs.bin --romfs cci\cxi4\romfs.bin
MD cci\cxi5
3dstool -xvtf cxi cci\5.cxi --header cci\cxi5\ncchheader.bin --exh cci\cxi5\exh.bin --ace cci\cxi5\ace.bin --plain cci\cxi5\plain.bin --exefs cci\cxi5\exefs.bin --romfs cci\cxi5\romfs.bin
MD cci\cxi6
3dstool -xvtf cxi cci\6.cxi --header cci\cxi6\ncchheader.bin --exh cci\cxi6\exh.bin --ace cci\cxi6\ace.bin --plain cci\cxi6\plain.bin --exefs cci\cxi6\exefs.bin --romfs cci\cxi6\romfs.bin
MD cci\cxi7
3dstool -xvtf cxi cci\7.cxi --header cci\cxi7\ncchheader.bin --exh cci\cxi7\exh.bin --ace cci\cxi7\ace.bin --plain cci\cxi7\plain.bin --exefs cci\cxi7\exefs.bin --romfs cci\cxi7\romfs.bin
3dstool -xvtf romfs cci\cxi0\romfs.bin --romfsdir cci\cxi0\romfs
3dstool -xvtf romfs cci\cxi1\romfs.bin --romfsdir cci\cxi1\romfs
3dstool -xvtf romfs cci\cxi2\romfs.bin --romfsdir cci\cxi2\romfs
3dstool -xvtf romfs cci\cxi3\romfs.bin --romfsdir cci\cxi3\romfs
3dstool -xvtf romfs cci\cxi4\romfs.bin --romfsdir cci\cxi4\romfs
3dstool -xvtf romfs cci\cxi5\romfs.bin --romfsdir cci\cxi5\romfs
3dstool -xvtf romfs cci\cxi6\romfs.bin --romfsdir cci\cxi6\romfs
3dstool -xvtf romfs cci\cxi7\romfs.bin --romfsdir cci\cxi7\romfs
PAUSE
