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



class PdfTextFont
{
public:
	int    charset = { 1 };
	QFont  font;
	double rotation = { 0.0 };
};


struct PdfGlyphStyle
{
public:
	QFont font = {};
	bool  fill;
	bool stroke;
	double rotation = { 0.0 };
	int    charset = { 1 };
	QString currColorFill = { "black" };
	QString currColorStroke = { "black" };
	int currStrokeShade{ 100 };
	QString toString(void) 
	{
		QTextStream result;
		result << "fill=" << fill << ":stroke=" << stroke << ":font=" << font.toString();
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

	QPointF pdfTextRegionBasenOrigin = QPointF({}, {});
	qreal maxHeight = {};
	qreal lineSpacing = { 1 };
	std::vector<PdfTextRegionLine> pdfTextRegionLines = std::vector<PdfTextRegionLine>();
	qreal maxWidth = {};
	QPointF lineBaseXY = QPointF({ }, { }); //updated with the best match left value from all the textRegionLines and the best bottom value from the textRegionLines.segments;
	QPointF lastXY = QPointF({}, {});
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
	void SetNewFontAndStyle(PdfGlyphStyle* fontAndSttle);
	std::vector<PdfGlyph> glyphs;
	bool isNew();
private:
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
		qDebug() << "charmode is:" << static_cast<int>(m_addCharMode) << " and it's being set to:" << static_cast<int>(mode);
		if ((m_addCharMode == AddCharMode::ADDCHARWITHNEWSTYLE || m_addCharMode == AddCharMode::ADDCHARWITHBASESTLYE) && mode == AddCharMode::ADDFIRSTCHAR)
		{
			qDebug() << "attempt to set addCharMode to  AddCharMode::ADDFIRSTCHAR  when it is already set to ddCharMode::ADDCHARWITHNEWSTYLE or  AddCharMode::ADDCHARWITHBASESTLYE which have a higher precedence, returning";
			return;
		}
		if (m_addCharMode == AddCharMode::ADDCHARWITHBASESTLYE && mode == AddCharMode::ADDCHARWITHNEWSTYLE)
		{
			qDebug() << "attempt to set addCharMode to  AddCharMode::ADDCHARWITHNEWSTYLE  when it is already set to ddCharMode::ADDCHARWITHBASESTLYE which has a higher precedence, returning";
			return;
		}
		m_addCharMode = mode;
	}


	PdfTextRegion&& activePdfTextRegion = PdfTextRegion(); //faster and cleaner than calling back on the vector all the time.
	void addPdfTextRegion();
	void addChar(GfxState* state, double x, double y, double dx, double dy, double originX, double originY, CharCode code, int nBytes, POPPLER_CONST_082 Unicode* u, int uLen);
	bool isChangeOnSameLine(QPointF newPosition);
	bool isNewLineOrRegion(QPointF newPosition);
	void setFillColour(QString fillColour);
	void setStrokeColour(QString strokleColour);
	void setPdfGlyphStyleFont(QFont font);
private:
	std::vector<PdfTextRegion> m_pdfTextRegions = std::vector<PdfTextRegion>();
	AddCharMode m_addCharMode = AddCharMode::ADDCHARWITHBASESTLYE;
	PdfGlyph AddCharCommon(GfxState* state, double x, double y, double dx, double dy, Unicode const* u, int uLen);
	PdfGlyph AddFirstChar(GfxState* state, double x, double y, double dx, double dy, double originX, double originY, CharCode code, int nBytes, Unicode const* u, int uLen);
	PdfGlyph AddBasicChar(GfxState* state, double x, double y, double dx, double dy, double originX, double originY, CharCode code, int nBytes, Unicode const* u, int uLen);
	PdfGlyph AddCharWithNewStyle(GfxState* state, double x, double y, double dx, double dy, double originX, double originY, CharCode code, int nBytes, Unicode const* u, int uLen);
	PdfGlyph AddCharWithPreviousStyle(GfxState* state, double x, double y, double dx, double dy, double originX, double originY, CharCode code, int nBytes, Unicode const* u, int uLen);
	PdfGlyph AddCharWithBaseStyle(GfxState* state, double x, double y, double dx, double dy, double originX, double originY, CharCode code, int nBytes, Unicode const* u, int uLen);
	PdfGlyphStyle m_pdfGlyphStyle = {};
	PdfGlyphStyle m_previousFontAndStyle = {};
};


class PdfTextOutputDev : public SlaOutputDev
{
public:
	PdfTextOutputDev(ScribusDoc* doc, QList<PageItem*>* Elements, QStringList* importedColors, int flags);
	virtual ~PdfTextOutputDev();

	void updateFont(GfxState* state) override;
	void updateFillColor(GfxState* state) override;
	void updateStrokeColor(GfxState* state) override;

	//----- text drawing
	void  beginTextObject(GfxState* state) override;
	void  endTextObject(GfxState* state) override;
	void  drawChar(GfxState* state, double /*x*/, double /*y*/, double /*dx*/, double /*dy*/, double /*originX*/, double /*originY*/, CharCode /*code*/, int /*nBytes*/, POPPLER_CONST_082 Unicode* /*u*/, int /*uLen*/) override;
	GBool beginType3Char(GfxState* /*state*/, double /*x*/, double /*y*/, double /*dx*/, double /*dy*/, CharCode /*code*/, POPPLER_CONST_082 Unicode* /*u*/, int /*uLen*/) override;
	void  endType3Char(GfxState* /*state*/) override;
	void  type3D0(GfxState* /*state*/, double /*wx*/, double /*wy*/) override;
	void  type3D1(GfxState* /*state*/, double /*wx*/, double /*wy*/, double /*llx*/, double /*lly*/, double /*urx*/, double /*ury*/) override;
	void updateTextMat(GfxState* state) override;
	void updateTextShift(GfxState* state, double shift) override;
	static size_t MatchingChars(QString s1, QString sp);
	QString _bestMatchingFont(QString PDFname);
	void _updateStyle(GfxState* state);
private:
	void setFillAndStrokeForPDF(GfxState* state, PageItem* text_node);
	void updateTextPos(GfxState* state) override;
	void renderTextFrame();
	void finishItem(PageItem* item);
	PdfTextRecognition m_pdfTextRecognition = {};
	PdfGlyphStyle m_pdfGlyphStyle = {};	
	PdfGlyphStyle m_previouisGlyphStyle = {};
	bool m_invalidatedStyle = true;
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
