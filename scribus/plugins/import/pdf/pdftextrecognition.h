/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
#ifndef PDFTEXTRECOGNITION_H
#define PDFTEXTRECOGNITION_H

#include <QSizeF>
#include <QStack>
#include <QString>

#include "pageitem.h"
#include "importpdfconfig.h"
#include "slaoutput.h"

#include <poppler/GfxState.h>
#include <poppler/CharCodeToUnicode.h>

struct PdfTextFont
{
	QString name = {};
	bool bold = { false };
	bool italic = { false };
	PdfTextFont(QString name, bool bold, bool italic)
	{
		this->name = name;
		this->bold = bold;
		this->italic = italic;
	}
	inline  QString toString()
	{
		return name + ":" + bold + ":" + italic;
	}

	inline bool operator< (const PdfTextFont& rhs) const { return name + ":" + bold + ":" + italic < rhs.name + ":" + rhs.bold + ":" + rhs.italic; }
	inline bool operator> (const PdfTextFont& rhs) const { return rhs.name + ":" + rhs.bold + ":" + rhs.italic < name + ":" + bold + ":" + italic; }
	inline bool operator<=(const PdfTextFont& rhs) const { return !(name + ":" + bold + ":" + italic > rhs.name + ":" + rhs.bold + ":" + rhs.italic); }
	inline bool operator>=(const PdfTextFont& rhs) const { return !(name + ":" + bold + ":" + italic < rhs.name + ":" + rhs.bold + ":" + rhs.italic); }
};


struct PdfGlyphStyle
{
public:
	ScFace face = { };
	bool  fill = { false };
	bool stroke = { false };
	double rotation = { 0.0 };
	int    charset = { 1 };
	QString currColorFill = { "0x00000000" };
	QString currColorStroke = { "0x00000000" };
	int currStrokeShade{ 100 };
	// font size in points;
	double pointSizeF = { 1.0 };
	double fontScaling = { 1.0 };
	QString toString(void) 
	{
		QTextStream result;
		result << "fill=" << fill << ":stroke=" << stroke << ":font=" << face.scName();
		result.flush();
		return *result.string();
	}
};

/* PDF TextBox Framework */
/*
* Holds all the details for each glyph in the text imported from the pdf file.
*
*/
struct PdfGlyph
{
	double dx;  // X advance value
	double dy;  // Y advance value
	double rise;    // Text rise parameter
	QChar code;   // UTF-16 coded character
};


class PdfTextRegionLine
{
public:
	qreal maxHeight = {};
	//we can probably use maxHeight for this.	
	qreal width = {};
	int glyphIndex = {};
	QPointF baseOrigin = QPointF({}, {});
	std::vector<PdfTextRegionLine> segments = std::vector<PdfTextRegionLine>();
	PdfGlyphStyle pdfGlyphStyle = { PdfGlyphStyle() };

};


class ModeArray
{
public:	
	qreal add(qreal value);
	qreal mode(void);
	void clear(void);
private:
	static const qreal m_lastMaxInvalid;
	qreal m_lastMax = { 0.0 };	
	std::map<qreal, int> m_modeArrayMap = std::map<qreal, int>();
};

class PdfTextRegion
{
public:
	enum class LineType
	{
		FIRSTPOINT,
		SAMELINE,
		STYLESUPERSCRIPT,
		STYLENORMALRETURN,
		STYLEBELOWBASELINE,
		NEWLINE,
		ENDOFLINE, //TODO: Implement an end of line test
		FAIL
	};
#
	/*
* the bounding box shape splines in percentage of width and height. In this case 100% as we want to clip shape to be the full TextBox width and height. */
	static constexpr double boundingBoxShape[32] = { 0.0,0.0
							,0.0,0.0
							,100.0,0.0
							,100.0,0.0
							,100.0,0.0
							,100.0,0.0
							,100.0,100.0
							,100.0,100.0
							,100.0,100.0
							,100.0,100.0
							,0.0,100.0
							,0.0,100.0
							,0.0,100.0
							,0.0,100.0
							,0.0,0.0
							,0.0,0.0
	};
	inline QPointF& pdfTextRegionBasenOrigin()
	{
		return m_pdfTextRegionBasenOrigin;
	}
	inline void setPdfTextRegionBasenOrigin(QPointF qPointF)
	{
		m_pdfTextRegionBasenOrigin = qPointF;
	}
	
	inline qreal& maxHeight()
	{
		return m_maxHeight;
	}
	inline void setMaxHeight(qreal real)
	{
		 m_maxHeight = real;
	}
	inline ModeArray& lineSpacing()
	{
		return m_lineSpacing;
	}
	/*
	inline void setLineSpacing(ModeArray modeArray)
	{
		m_lineSpacing = modeArray;
	}
	*/
	inline const qreal& fontAssending()
	{
		return m_fontAssending;
	}
	inline void setFontAssending(qreal real)
	{
		m_fontAssending = real;
	}
	inline std::vector<PdfTextRegionLine>& pdfTextRegionLines()
	{
		return m_pdfTextRegionLines;
	}
	/*
	inline void setPdfTextRegionLines(std::vector<PdfTextRegionLine> pdfTextRegionLines)
	{
		m_pdfTextRegionLines = pdfTextRegionLines;
	}
	*/
	inline qreal& maxWidth()
	{
		return m_maxWidth;
	}
	inline void setMaxWidth(qreal real)
	{
		m_maxWidth = real;
	}
	inline QPointF& lineBaseXY()
	{
		return m_lineBaseXY;
	}
	inline void setLineBaseXY(QPointF qPointF)
	{
		m_lineBaseXY = qPointF;
	}
	inline QPointF& lastXY()
	{
		return m_lastXY;
	}
	inline void setLastXY(QPointF qPointF)
	{
		m_lastXY = qPointF;
	}

	static bool collinear(qreal a, qreal b);
	bool isCloseToX(qreal x1, qreal x2);
	bool isCloseToY(qreal y1, qreal y2);
	bool adjunctLesser(qreal testY, qreal lastY, qreal baseY);
	bool adjunctGreater(qreal testY, qreal lastY, qreal baseY);
	PdfTextRegion::LineType linearTest(QPointF point, bool xInLimits, bool yInLimits);
	PdfTextRegion::LineType isRegionConcurrent(QPointF newPoint);
	PdfTextRegion::LineType moveToPoint(QPointF newPoint);
	PdfTextRegion::LineType addGlyphAtPoint(QPointF newGlyphPoint, PdfGlyph new_glyph);
	void renderToTextFrame(PageItem* textNode);
	void doBreaksAndSpaces(void);
	void insertChar(std::vector<PdfTextRegionLine>::iterator textRegionLineItterator, int increment, QChar qChar);
	void setNewFontAndStyle(PdfGlyphStyle* fontAndSttle);
	std::vector<PdfGlyph> glyphs;
	bool isNew();
private:
	QPointF m_pdfTextRegionBasenOrigin = QPointF({}, {});
	qreal m_maxHeight = {};
	ModeArray m_lineSpacing = ModeArray();
	qreal m_fontAssending = { 1.0 };
	std::vector<PdfTextRegionLine> m_pdfTextRegionLines = std::vector<PdfTextRegionLine>();
	qreal m_maxWidth = {};
	QPointF m_lineBaseXY = QPointF({ }, { }); //updated with the best match left value from all the textRegionLines and the best bottom value from the textRegionLines.segments;
	QPointF m_lastXY = QPointF({}, {});
	PdfGlyphStyle* m_newFontStyleToApply = nullptr;          
	PdfTextRegion::LineType m_lastMode = PdfTextRegion::LineType::FIRSTPOINT;
};

class PdfTextRecognition
{
public:
	PdfTextRecognition();
	~PdfTextRecognition();

	enum class AddCharMode
	{
		ADDFIRSTCHAR,
		ADDBASICCHAR,
		ADDCHARWITHNEWSTYLE,
		ADDCHARWITHPREVIOUSSTYLE,
		ADDCHARWITHBASESTLYE
	};

	void setCharMode(AddCharMode mode)
	{
		//qDebug() << "charmode is:" << static_cast<int>(m_addCharMode) << " and it's being set to:" << static_cast<int>(mode);
		if ((m_addCharMode == AddCharMode::ADDCHARWITHNEWSTYLE || m_addCharMode == AddCharMode::ADDCHARWITHBASESTLYE) && mode == AddCharMode::ADDFIRSTCHAR)
		{
			//qDebug() << "attempt to set addCharMode to  AddCharMode::ADDFIRSTCHAR  when it is already set to ddCharMode::ADDCHARWITHNEWSTYLE or  AddCharMode::ADDCHARWITHBASESTLYE which have a higher precedence, returning";
			return;
		}
		if (m_addCharMode == AddCharMode::ADDCHARWITHBASESTLYE && mode == AddCharMode::ADDCHARWITHNEWSTYLE)
		{
			//qDebug() << "attempt to set addCharMode to  AddCharMode::ADDCHARWITHNEWSTYLE  when it is already set to ddCharMode::ADDCHARWITHBASESTLYE which has a higher precedence, returning";
			return;
		}
		m_addCharMode = mode;
	}


	PdfTextRegion&& activePdfTextRegion = PdfTextRegion(); //faster and cleaner than calling back on the vector all the time.
	void addPdfTextRegion();
	void addChar(GfxState* state, double x, double y, double dx, double dy, double originX, double originY, CharCode code, int nBytes, POPPLER_CONST_082 Unicode* u, int uLen);
	bool isChangeOnSameLine(QPointF newPosition);
	bool isNewLine(QPointF newPosition);	
	bool isNewRegion(QPointF newPosition);
	void setFillColour(QString fillColour);
	void setStrokeColour(QString strokleColour);
	void setPdfGlyphStyleFace(ScFace& face);
	void setPdfGlyphStyleSizeF(double pointSizeF);
	void setPdfGlyphStyleScale(double fontScaling);

	bool newFontAndStyle = false;
private:
	std::vector<PdfTextRegion*> m_pdfTextRegions = std::vector<PdfTextRegion*>();
	AddCharMode m_addCharMode = AddCharMode::ADDCHARWITHBASESTLYE;
	PdfGlyph AddCharCommon(GfxState* state, double x, double y, double dx, double dy, Unicode const* u, int uLen);
	PdfGlyph AddFirstChar(GfxState* state, double x, double y, double dx, double dy, double originX, double originY, CharCode code, int nBytes, Unicode const* u, int uLen);
	PdfGlyph AddBasicChar(GfxState* state, double x, double y, double dx, double dy, double originX, double originY, CharCode code, int nBytes, Unicode const* u, int uLen);
	PdfGlyph AddCharWithNewStyle(GfxState* state, double x, double y, double dx, double dy, double originX, double originY, CharCode code, int nBytes, Unicode const* u, int uLen);
	PdfGlyph AddCharWithPreviousStyle(GfxState* state, double x, double y, double dx, double dy, double originX, double originY, CharCode code, int nBytes, Unicode const* u, int uLen);
	PdfGlyph AddCharWithBaseStyle(GfxState* state, double x, double y, double dx, double dy, double originX, double originY, CharCode code, int nBytes, Unicode const* u, int uLen);		
	PdfGlyphStyle m_pdfGlyphStyle = {};
	PdfGlyphStyle m_previousFontAndStyle = {};	
	bool m_addWhiteSpace = false;
};

class PdfTextOutputDev : public SlaOutputDev
{
public:
	PdfTextOutputDev(ScribusDoc* doc, QList<PageItem*>* Elements, QStringList* importedColors, int flags);
	virtual ~PdfTextOutputDev();

	void updateFont(GfxState* state) override;	
	bool faceMatches(ScFace& face1, ScFace& face2);
	void updateFillColor(GfxState* state) override;
	void updateStrokeColor(GfxState* state) override;

	//----- text drawing
	void beginTextObject(GfxState* state) override;
	void endTextObject(GfxState* state) override;
	void drawChar(GfxState* state, double x, double y, double dx, double dy, double originX, double originY, CharCode code, int nBytes, POPPLER_CONST_082 Unicode* u, int uLen) override;
	GBool beginType3Char(GfxState* /*state*/, double /*x*/, double /*y*/, double /*dx*/, double /*dy*/, CharCode /*code*/, POPPLER_CONST_082 Unicode* /*u*/, int /*uLen*/) override;
	void endType3Char(GfxState* /*state*/) override;
	void type3D0(GfxState* /*state*/, double /*wx*/, double /*wy*/) override;
	void type3D1(GfxState* /*state*/, double /*wx*/, double /*wy*/, double /*llx*/, double /*lly*/, double /*urx*/, double /*ury*/) override;
	void updateTextMat(GfxState* state) override;
	void updateTextShift(GfxState* state, double shift) override;
	static size_t MatchingChars(QString s1, QString sp);
	//QString bestMatchingFont(QString PDFname);
private:
	//void setFillAndStrokeForPDF(GfxState* state, PageItem* text_node);
	void updateTextPos(GfxState* state) override;
	void renderTextFrame();
	void finishItem(PageItem* item);
	ScFace* getCachedFont(GfxFont* font);
	ScFace* cachedFont(GfxFont* font);	
	ScFace* matchScFaceToFamilyAndStyle(const QString& fontName, const QString& font_style_lowercase, bool bold, bool italic);
	ScFace* makeFont(GfxFont* font, QString cs_font_family, QString font_style_lowercase);	

	std::map<PdfTextFont, ScFace* > m_fontMap = std::map<PdfTextFont, ScFace* >();
	PdfTextRecognition m_pdfTextRecognition = {};
	PdfGlyphStyle m_pdfGlyphStyle = {};	
	PdfGlyphStyle m_previouisGlyphStyle = {};	
	QString m_lastFontSpecification = {};
	QTransform m_textMatrix = {};
	double m_fontScaling = { 1.0 };
	bool m_needFontUpdate = true;
	QStringList m_availableFontNames = {};
};

/**
 * This array holds info about translating font weight names to more or less CSS equivalents
 */
static char* font_weight_translator[][2] = {
	{(char*)"bold",        (char*)"bold"},
	{(char*)"light",       (char*)"300"},
	{(char*)"black",       (char*)"900"},
	{(char*)"heavy",       (char*)"900"},
	{(char*)"ultrabold",   (char*)"800"},
	{(char*)"extrabold",   (char*)"800"},
	{(char*)"demibold",    (char*)"600"},
	{(char*)"semibold",    (char*)"600"},
	{(char*)"medium",      (char*)"500"},
	{(char*)"book",        (char*)"normal"},
	{(char*)"regular",     (char*)"normal"},
	{(char*)"roman",       (char*)"normal"},
	{(char*)"normal",      (char*)"normal"},
	{(char*)"ultralight",  (char*)"200"},
	{(char*)"extralight",  (char*)"200"},
	{(char*)"thin",        (char*)"100"}
};


#endif
