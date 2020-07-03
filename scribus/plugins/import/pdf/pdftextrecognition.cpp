/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

#include "pdftextrecognition.h"
/*
#ifndef DEBUG_TEXT_IMPORT
	#define DEBUG_TEXT_IMPORT
	#define DEBUG_TEXT_IMPORT_FONTS
#endif
*/
#include <qfontdatabase.h>

/*
*	constructor, initialize the textRegions vector and set the addChar mode
*/
PdfTextRecognition::PdfTextRecognition()
{
	m_pdfTextRegions.push_back(activePdfTextRegion);
	setCharMode(AddCharMode::ADDCHARWITHBASESTLYE);
}

/*
*	nothing to do in the destructor yet
*/
PdfTextRecognition::~PdfTextRecognition()
{
}

/*
*	add a new text region and make it the active region
*/
void PdfTextRecognition::addPdfTextRegion()
{
	activePdfTextRegion = 
		PdfTextRegion();
	m_pdfTextRegions.push_back(activePdfTextRegion);
	setCharMode(PdfTextRecognition::AddCharMode::ADDCHARWITHBASESTLYE);
}

/*
*	function called via integration with poppler's addChar callback. It decides how to add the charter based on the mode that is set
*/
void PdfTextRecognition::addChar(GfxState* state, double x, double y, double dx, double dy, double originX, double originY, CharCode code, int nBytes, POPPLER_CONST_082 Unicode* u, int uLen)
{

	switch (this->m_addCharMode)
	{
	case AddCharMode::ADDFIRSTCHAR:
		AddFirstChar(state, x, y, dx, dy, originX, originY, code, nBytes, u, uLen);
		break;
	case AddCharMode::ADDBASICCHAR:
		AddBasicChar(state, x, y, dx, dy, originX, originY, code, nBytes, u, uLen);
		break;
	case AddCharMode::ADDCHARWITHNEWSTYLE:
		AddCharWithNewStyle(state, x, y, dx, dy, originX, originY, code, nBytes, u, uLen);
		break;
	case AddCharMode::ADDCHARWITHPREVIOUSSTYLE:
		AddCharWithPreviousStyle(state, x, y, dx, dy, originX, originY, code, nBytes, u, uLen);
		break;
	case AddCharMode::ADDCHARWITHBASESTLYE:
		AddCharWithBaseStyle(state, x, y, dx, dy, originX, originY, code, nBytes, u, uLen);
		break;
	}
}

bool PdfTextRecognition::isChangeOnSameLine(QPointF newPosition)
{
	return false;
}

/*
*	basic test to see if the point lies in a new line or region
*/
bool PdfTextRecognition::isNewLineOrRegion(QPointF newPosition)
{
	auto lineRelationship = activePdfTextRegion.isRegionConcurrent(newPosition);
	return lineRelationship == PdfTextRegion::LineType::NEWLINE || lineRelationship == PdfTextRegion::LineType::ENDOFLINE || lineRelationship == PdfTextRegion::LineType::FAIL;
}

void PdfTextRecognition::setFillColour(QString fillColour)
{
	m_pdfGlyphStyle.currColorFill = fillColour;
	setCharMode(AddCharMode::ADDCHARWITHNEWSTYLE);
}

void PdfTextRecognition::setStrokeColour(QString strokleColour)
{
	m_pdfGlyphStyle.currColorStroke = strokleColour;
	setCharMode(AddCharMode::ADDCHARWITHNEWSTYLE);
}

void PdfTextRecognition::setPdfGlyphStyleFont(QFont font)
{
	m_pdfGlyphStyle.font = font;
	setCharMode(AddCharMode::ADDCHARWITHNEWSTYLE);
}

void PdfTextRecognition::doBreaksAnbdSpaces(void)
{
	int increment = 0;
	for(auto line = activePdfTextRegion.pdfTextRegionLines.begin(); line < activePdfTextRegion.pdfTextRegionLines.end() - 1; line++)
	{
		increment++;
		if ((*line).width < activePdfTextRegion.maxWidth - 20)
		{
			insertChar(line, increment, QChar::SpecialCharacter::LineSeparator);
		}
		else
		{
			insertChar(line, increment, ' ');
		}
	}
	activePdfTextRegion.pdfTextRegionLines.back().glyphIndex += increment;
	for (auto segment = activePdfTextRegion.pdfTextRegionLines.back().segments.begin(); segment < activePdfTextRegion.pdfTextRegionLines.back().segments.end(); segment++)
	{
		(*segment).glyphIndex += increment;
	}
}

void PdfTextRecognition::insertChar(std::vector<PdfTextRegionLine>::iterator textRegionLineItterator, int increment, QChar qChar)
{
	auto glyphItterator = (activePdfTextRegion.glyphs.begin() + (*textRegionLineItterator).segments.back().glyphIndex + increment);
	PdfGlyph newGlyph = PdfGlyph();
	newGlyph.code = qChar;
	newGlyph.dx = 10;
	//no dx or dy as were only inserting spaces and new lines
	activePdfTextRegion.glyphs.insert(glyphItterator, newGlyph);

	
	(*textRegionLineItterator).segments.back().glyphIndex++;
	if(increment > 1){
		(*textRegionLineItterator).glyphIndex += increment - 1;
		for (auto segment = (*textRegionLineItterator).segments.begin(); segment < (*textRegionLineItterator).segments.end(); segment++)
		{
			(*segment).glyphIndex += increment - 1;
		}
	}
}



/*
*	basic functionality to be performed when addChar is called
*	FIXME: what to do when uLen != 1
*/
PdfGlyph PdfTextRecognition::AddCharCommon(GfxState* state, double x, double y, double dx, double dy, Unicode const* u, int uLen)
{
	//qDebug() << "AddBasicChar() '" << u << " : " << uLen;
	PdfGlyph newGlyph;
	newGlyph.dx = dx;
	newGlyph.dy = dy;

	// Convert the character to UTF-16 since that's our SVG document's encoding
	//m_pdfGlyphStyle.currColorStroke = getColor(state->getStrokeColorSpace(), m_pdfGlyphStyle.currColorStroke, m_pdfGlyphStyle.currStrokeShade);

	if (uLen > 1)
		qDebug() << "FIXME: AddBasicChar() '" << u << " : " << uLen;
	newGlyph.code = static_cast<char16_t>(u[uLen - 1]);
	newGlyph.rise = state->getRise();
	return newGlyph;
}

/*
*	Tell the text region to add a glyph so that line segments and regions be created
*	If the character being added is the first character in a textregion or after a change in positioning or styles or the end of a line
*	The success == TextRegion::LineType::FAIL test is an invariant test that should never pass. if a rogue glyph is detected then it means there is a bug in the logic probably in TextRegion::addGlyphAtPoint or TextRegion::linearTest or TextRegion::moveToPoint
*/
PdfGlyph PdfTextRecognition::AddFirstChar(GfxState* state, double x, double y, double dx, double dy, double originX, double originY, CharCode code, int nBytes, Unicode const* u, int uLen)
{
	//qDebug() << "AddFirstChar() '" << u << " : " << uLen;
	PdfGlyph newGlyph = PdfTextRecognition::AddCharCommon(state, x, y, dx, dy, u, uLen);
	activePdfTextRegion.glyphs.push_back(newGlyph);
	setCharMode(AddCharMode::ADDBASICCHAR);
	auto success = activePdfTextRegion.addGlyphAtPoint(QPointF(x, y), newGlyph);
	if (success == PdfTextRegion::LineType::FAIL)
		qDebug("FIXME: Rogue glyph detected, this should never happen because the cursor should move before glyphs in new regions are added.");
	return newGlyph;
}

/*
*	just add a character to the textregion without doing anything special
*/
PdfGlyph PdfTextRecognition::AddBasicChar(GfxState* state, double x, double y, double dx, double dy, double originX, double originY, CharCode code, int nBytes, Unicode const* u, int uLen)
{
	//qDebug() << "AddBasicChar() x'" << x << "y:" << y << "dx:" << dx << "dy:" << dy << "originX:" << originX << "originY" << originY << "code:" << code << "nBytes:" << nBytes << "Unicode:" << u << " uLen: " << uLen;
	PdfGlyph newGlyph = AddCharCommon(state, x, y, dx, dy, u, uLen);
	activePdfTextRegion.lastXY = QPointF(x, y);
	activePdfTextRegion.glyphs.push_back(newGlyph);
	return newGlyph;
}

/*
*	Apply a new style to this glyph ands glyphs that follow and add it to the style stack
*/
PdfGlyph PdfTextRecognition::AddCharWithNewStyle(GfxState* state, double x, double y, double dx, double dy, double originX, double originY, CharCode code, int nBytes, Unicode const* u, int uLen)
{
	//qDebug() << "AddCharWithNewStyle() x'" << x << "y:" << y << "dx:" << dx << "dy:" << dy << "originX:" << originX << "originY" << originY << "code:" << code << "nBytes:" << nBytes << "Unicode:" << u << " uLen: " << uLen;
	auto newGlyph = AddCharCommon(state, x, y, dx, dy, u, uLen);	
	auto success = activePdfTextRegion.moveToPoint(QPointF( x, y ));
	if (success == PdfTextRegion::LineType::FAIL)
		qDebug() << "moveTo just failed, maybe we shouldn't be calling addGlyph if moveto has just failed.";
	activePdfTextRegion.glyphs.push_back(newGlyph);
	setCharMode(AddCharMode::ADDBASICCHAR);
	activePdfTextRegion.SetNewFontAndStyle(&m_pdfGlyphStyle);
	success = activePdfTextRegion.addGlyphAtPoint(QPointF(x, y), newGlyph);
	if (success == PdfTextRegion::LineType::FAIL)
		qDebug("FIXME: Rogue glyph detected, this should never happen because the cursor should move before glyphs in new regions are added.");
	return newGlyph;
}

/*
*	return to the previous style on the style stack
*	TODO: Currently not implemented, just stub code
*/
PdfGlyph PdfTextRecognition::AddCharWithPreviousStyle(GfxState* state, double x, double y, double dx, double dy, double originX, double originY, CharCode code, int nBytes, Unicode const* u, int uLen)
{
	//qDebug() << "AddCharWithPreviousStyle() '" << u << " : " << uLen;
	auto newGlyph = AddCharCommon(state, x, y, dx, dy, u, uLen);
	activePdfTextRegion.glyphs.push_back(newGlyph);
	return newGlyph;
}

/*
*	return to the previous style on the style stack
*/
PdfGlyph PdfTextRecognition::AddCharWithBaseStyle(GfxState* state, double x, double y, double dx, double dy, double originX, double originY, CharCode code, int nBytes, Unicode const* u, int uLen)
{
	//qDebug() << "AddCharWithBaseStyle() '" << u << " : " << uLen;
	PdfGlyph newGlyph = PdfTextRecognition::AddCharCommon(state, x, y, dx, dy, u, uLen);	
	setCharMode(AddCharMode::ADDBASICCHAR);
	auto success = activePdfTextRegion.moveToPoint(QPointF(x, y));
	activePdfTextRegion.SetNewFontAndStyle(&m_pdfGlyphStyle);
	activePdfTextRegion.glyphs.push_back(newGlyph);
	success = activePdfTextRegion.addGlyphAtPoint(QPointF(x, y), newGlyph);
	if (success == PdfTextRegion::LineType::FAIL)
		qDebug("FIXME: Rogue glyph detected, this should never happen because the cursor should move before glyphs in new regions are added.");
	return newGlyph;
}

/*
*	functions to do fuzzy testing on the proximity of points to one another and in relation to the textregion
*	FIXME: There should be a parameter in the UI to set the matching tolerance but hard code for now
*/

/*
*	In geometry, collinearity of a set of points is the property of their lying on a single line. A set of points with this property is said to be collinear.
*	In greater generality, the term has been used for aligned objects, that is, things being "in a line" or "in a row".
*	PDF never deviates from the line when it comes to collinear, but allow for 1pixel of divergence
*/
bool PdfTextRegion::collinear(qreal a, qreal b)
{
	return abs(a - b) < 1 ? true : false;
}

/*
*	like collinear but we allow a deviation of 6 text widths from between positions or 1 text width from the textregion's x origin
*   FIXME: This should use the char width not linespacing which is y
*/
bool PdfTextRegion::isCloseToX(qreal x1, qreal x2)
{
	
	return (abs(x2 - x1) <= lineSpacing * 6) || (abs(x1 - this->pdfTextRegionBasenOrigin.x()) <= lineSpacing);
}

/*
*	like collinear but we allow a deviation of 3 text heights downwards but none upwards
*/
bool PdfTextRegion::isCloseToY(qreal y1, qreal y2)
{	
	return (y2 - y1) >= 0 && y2 - y1 <= lineSpacing * 3;	
}

/*
*	less than, page upwards, the last y value but bot more than the line spacing less, could also use the base line of the last line to be more accurate
*/
bool PdfTextRegion::adjunctLesser(qreal testY, qreal lastY, qreal baseY)
{
	return (testY > lastY
		&& testY <= baseY + lineSpacing
		&& lastY <= baseY + lineSpacing);
}

/*
*	greater, page downwards, than the last y value but not more than 3/4 of a line space below baseline
*/
bool PdfTextRegion::adjunctGreater(qreal testY, qreal lastY, qreal baseY)
{
	return (testY <= lastY
		&& testY >= baseY - lineSpacing * 0.75
		&& lastY != baseY);
}

/*
*	Test to see if the point is part of the current block of text or is part of a new block of text(FrameworkLineTests::FAIL).
*	checks to see if it's the first point, on the same line, super and sub script, returning to baseline from super script or if we are on a new line.
*	matching is fuzzy allowing for multiple linespaces and text indentation. right hand justifications still needs to be dealt with as well as identifying if we are on a new paragraph
*	tests are weaker if we are on the first and moving to the second lines of text because we don't have enough information about how the text in the region
*	is formatted and in those cases the linespace is taken to be twice the glyph width.
*	FIXME: This needs fixing when font support is added and the ascending and descending values for the font should be used instead of the glyphs width.
*	TODO: support LineType::STYLESUBSCRIPT
*	TODO: support NEWLINE new paragraphs with multiple linespaces and indented x insteads of just ignoring the relative x position
*	TODO: I don't know if the invariant qDebug cases should always report an error or only do so when DEBUG_TEXT_IMPORT is defined. My feeling is they should always report because it meanms something has happened that shouldn't have and it's useful feedback.
*/
PdfTextRegion::LineType PdfTextRegion::linearTest(QPointF point, bool xInLimits, bool yInLimits)
{
	if (collinear(point.y(), lastXY.y()))
	{
		if (collinear(point.x(), lastXY.x()))
			return LineType::FIRSTPOINT;
		else if (xInLimits)
			return LineType::SAMELINE;
#ifdef DEBUG_TEXT_IMPORT
		qDebug() << "FIRSTPOINT/SAMELINE oops:" << "point:" << point << " pdfTextRegioBasenOrigin:" << pdfTextRegionBasenOrigin << " baseline:" << this->lineBaseXY << " lastXY:" << lastXY << " linespacing:" << lineSpacing << " pdfTextRegionLines.size:" << pdfTextRegionLines.size();
#endif
	}
	else if (adjunctLesser(point.y(), lastXY.y(), lineBaseXY.y()))
		return LineType::STYLESUPERSCRIPT;
	else if (adjunctGreater(point.y(), lastXY.y(), lineBaseXY.y()))
		if (collinear(point.y(), lineBaseXY.y()))
			return LineType::STYLENORMALRETURN;
		else
			return LineType::STYLESUPERSCRIPT;
	else if (isCloseToX(point.x(), pdfTextRegionBasenOrigin.x()))
		if (isCloseToY(point.y(), lastXY.y()) && !collinear(point.y(), lastXY.y()))
			if (pdfTextRegionLines.size() >= 2)
				return LineType::NEWLINE;
			else if (pdfTextRegionLines.size() == 1)
				return LineType::NEWLINE;
			#ifdef DEBUG_TEXT_IMPORT
				qDebug() << "NEWLINE oops2:" << "point:" << point << " pdfTextRegionBasenOrigin:" << pdfTextRegionBasenOrigin << " baseline:" << this->lineBaseXY << " lastXY:" << lastXY << " linespacing:" << lineSpacing << " pdfTextRegionLines.size:" << pdfTextRegionLines.size();
			#endif
		#ifdef DEBUG_TEXT_IMPORT
			qDebug() << "NEWLINE oops:" << "point:" << point << " pdfTextRegioBasenOrigin:" << pdfTextRegionBasenOrigin << " baseline:" << this->lineBaseXY << " lastXY:" << lastXY << " linespacing:" << lineSpacing << " textPdfRegionLines.size:" << pdfTextRegionLines.size();
		#endif
	#ifdef DEBUG_TEXT_IMPORT //This isn't an invariant case like the others, we actually expect this to happen some of the time
		qDebug() << "FAILED with oops:" << "point:" << point << " pdfTextRegioBasenOrigin:" << pdfTextRegionBasenOrigin << " baseline:" << this->lineBaseXY << " lastXY:" << lastXY << " linespacing:" << lineSpacing << " textPdfRegionLines.size:" << pdfTextRegionLines.size();
	#endif
	return LineType::FAIL;
}

/*
*	Perform some fuzzy checks to see if newPoint can reasonably be ascribed to the current textframe.
*	FIXME: It may be that move and addGlyph need different versions of isCloseToX and isCloseToY but keep them the same just for now
*/
PdfTextRegion::LineType PdfTextRegion::isRegionConcurrent(QPointF newPoint)
{
	if (glyphs.empty())
	{
		lineBaseXY = newPoint;
		lastXY = newPoint;
	}

	bool xInLimits = isCloseToX(newPoint.x(), lastXY.x());
	bool yInLimits = isCloseToY(newPoint.y(), lastXY.y());
	LineType pass = linearTest(newPoint, xInLimits, yInLimits);
	return pass;
}

/*
*	Move the position of the cursor to a new point,
*	test if that point is within the current textframe or within a new textframe.
*	initialize the textregion and setup lines and segments
*	TODO: iscloseto x and y may need to be different from addGlyph but use the common isRegionbConcurrent for now
*		need to check to see if we are creating a new paragraph or not.
*		basically if the cursor is returned to x origin before it reached x width.
*		Also needs to have support for rotated text, but I expect I'll add this by removing the text rotation
*		from calls to movepoint and addGlyph and instead rotating the whole text region as a block
*/
PdfTextRegion::LineType PdfTextRegion::moveToPoint(QPointF newPoint)
{
	//qDebug() << "moveToPoint: " << newPoint;
	if (glyphs.empty())
	{
		lineBaseXY = newPoint;
		lastXY = newPoint;
	}
	LineType mode = isRegionConcurrent(newPoint);
	if (mode == LineType::FAIL)
		return mode;

	if (mode == LineType::FIRSTPOINT && glyphs.size() != 0)
	{
		mode = LineType::SAMELINE;
	}

	// have we been called twice with no change
	if (!pdfTextRegionLines.empty() && pdfTextRegionLines.back().segments.back().glyphIndex >= glyphs.size() - 1 && lastXY == newPoint)
	{
		return m_lastMode;
	}

	PdfTextRegionLine* pdfTextRegionLine = nullptr;
	if (mode == LineType::NEWLINE || mode == LineType::FIRSTPOINT)
	{
		if (mode != LineType::FIRSTPOINT || pdfTextRegionLines.empty())
			pdfTextRegionLines.push_back(PdfTextRegionLine());
		pdfTextRegionLine = &pdfTextRegionLines.back();		
		pdfTextRegionLine->glyphIndex = glyphs.size();
		pdfTextRegionLine->baseOrigin = newPoint;
		lineBaseXY = newPoint;
		if (mode == LineType::NEWLINE)
		{
			pdfTextRegionLine->maxHeight = abs(newPoint.y() - lastXY.y());
			if (pdfTextRegionLines.size() == 2)
				lineSpacing = abs(newPoint.y() - lastXY.y()) + 1;
		}
	}

	pdfTextRegionLine = &pdfTextRegionLines.back();
	if (mode != LineType::FIRSTPOINT && mode != LineType::NEWLINE && pdfTextRegionLines.back().segments.back().glyphIndex < glyphs.size() - 1)
	{
		pdfTextRegionLines.back().segments.back().glyphIndex = glyphs.size() - 1;
	}
	if ((mode == LineType::FIRSTPOINT && pdfTextRegionLine->segments.empty()) || mode == LineType::NEWLINE
		|| mode != LineType::FIRSTPOINT && pdfTextRegionLine->segments[0].glyphIndex != pdfTextRegionLine->glyphIndex)
	{
		PdfTextRegionLine newSegment = PdfTextRegionLine{};
		newSegment.glyphIndex = glyphs.size();
		pdfTextRegionLine->segments.push_back(newSegment);

		if (pdfTextRegionLines.size() > 1 && pdfTextRegionLine->segments.size() == 1)
		{
			pdfTextRegionLine->pdfGlyphStyle = pdfTextRegionLines[pdfTextRegionLines.size() - 2].segments.back().pdfGlyphStyle;	
			pdfTextRegionLine->segments.back().pdfGlyphStyle = pdfTextRegionLine->pdfGlyphStyle;
		}
	}

	PdfTextRegionLine* segment = &pdfTextRegionLine->segments.back();
	segment->baseOrigin = newPoint;
	segment->maxHeight = (mode == LineType::STYLESUPERSCRIPT) ?
		abs(lineSpacing - (newPoint.y() - lastXY.y())) :
		pdfTextRegionLines.back().maxHeight;

	if (mode != LineType::NEWLINE && mode != LineType::FIRSTPOINT)
	{
		pdfTextRegionLines.back().segments.back().width = abs(pdfTextRegionLines.back().segments.back().baseOrigin.x() - newPoint.x());
		pdfTextRegionLine = &pdfTextRegionLines.back();
		pdfTextRegionLine->width = abs(pdfTextRegionLine->baseOrigin.x() - newPoint.x());
	}

	maxHeight = abs(pdfTextRegionBasenOrigin.y() - newPoint.y()) > maxHeight ? abs(pdfTextRegionBasenOrigin.y() - newPoint.y()) : maxHeight;
	
	lastXY = newPoint;
	m_lastMode = mode;
	return mode;
}

/*
*	Add a new glyph to the current line segment, lines and segments should already have been setup by the
*	moveto function which should generally be called prior to addGlyph to setup the lines and segments correctly.
*	does some basic calculations to determine and save withs and heights and linespacings of texts etc...
*	FIXME: these need to be changed to use the mode average of all glyphs added to the text frame instead of just picking the first ones we come across
*		the mode average can also be used to determine the base font style when fonts are added
*		left and right hand margins however need to use the maximum and minimum, support for right hand justification
*		and centered text needs to be added as we only support left and fully justified at the moment.
*		Approximated heights and widths and linespaces need to use the correct font data when font support has been added,
*		but for now just use the x advance value. using font data should also allow for the support of rotated text that may use a mixture of x and y advance
*/
PdfTextRegion::LineType PdfTextRegion::addGlyphAtPoint(QPointF newGlyphPoint, PdfGlyph newGlyph)
{
	//qDebug() << "addGlyphAtPoint: newGlyphPoint:" << newGlyphPoint << " char:" << (QChar)newGlyph.code;
	QPointF movedGlyphPoint = QPointF(newGlyphPoint.x() + newGlyph.dx, newGlyphPoint.y() + newGlyph.dy);
	if (glyphs.size() == 1)
	{
		QFontMetrics qFontMetrics = QFontMetrics(m_newFontStyleToApply->font);
		lineSpacing = qFontMetrics.height();
		lastXY = newGlyphPoint;
		lineBaseXY = newGlyphPoint;
	}

	LineType mode = m_lastMode;
	if(glyphs.size() == 1 || lastXY != newGlyphPoint)
		mode = isRegionConcurrent(newGlyphPoint);
	if (mode == LineType::FAIL)
		return mode;

	if (mode == LineType::FIRSTPOINT && glyphs.size() > 1)
	{
		mode = LineType::SAMELINE;
	}
	maxHeight = abs(pdfTextRegionBasenOrigin.y() - movedGlyphPoint.y()) + lineSpacing > maxHeight ? abs(pdfTextRegionBasenOrigin.y() - movedGlyphPoint.y()) + lineSpacing : maxHeight;
	PdfTextRegionLine* pdfTextRegionLine = &pdfTextRegionLines.back();
	if (mode == LineType::NEWLINE || mode == LineType::FIRSTPOINT)
	{
		
		pdfTextRegionLine->baseOrigin = QPointF(pdfTextRegionBasenOrigin.x(), newGlyphPoint.y());
	}

	PdfTextRegionLine* segment = &pdfTextRegionLine->segments.back();
	segment->width = abs(movedGlyphPoint.x() - segment->baseOrigin.x());
	segment->glyphIndex = glyphs.size() - 1;
	if (m_newFontStyleToApply)	
		segment->pdfGlyphStyle = *m_newFontStyleToApply;

	qreal thisHeight = pdfTextRegionLines.size() > 1 ?
		abs(newGlyphPoint.y() - pdfTextRegionLines[pdfTextRegionLines.size() - 2].baseOrigin.y()) :
		newGlyph.dx;
	segment->maxHeight = thisHeight > segment->maxHeight ? thisHeight : segment->maxHeight;
	pdfTextRegionLine->maxHeight = pdfTextRegionLine->maxHeight > thisHeight ? pdfTextRegionLine->maxHeight : thisHeight;
	pdfTextRegionLine->width = abs(movedGlyphPoint.x() - pdfTextRegionLine->baseOrigin.x());

	maxWidth = pdfTextRegionLine->width > maxWidth ? pdfTextRegionLine->width : maxWidth;
	if (pdfTextRegionLine->segments.size() == 1)
		lineBaseXY = pdfTextRegionLine->baseOrigin;

	lastXY = movedGlyphPoint;
	return mode;
}

/*
*	Render the text region to the frame,
*	TODO: add support for rotated text
*/
void PdfTextRegion::renderToTextFrame(PageItem* textNode)
{
	ParagraphStyle *baseParagraphStyle = &(textNode->changeCurrentStyle());
	baseParagraphStyle->setLineSpacingMode(ParagraphStyle::LineSpacingMode::FixedLineSpacing);
	baseParagraphStyle->setLineSpacing(this->lineSpacing * 0.75);
	textNode->changeCurrentStyle() = *baseParagraphStyle;
	
	textNode->setWidthHeight(this->maxWidth, this->maxHeight);
	QString bodyText = "";
	auto itterator = this->pdfTextRegionLines.begin();
	for (int i = 0; i < pdfTextRegionLines.size(); i++)
	{
		bodyText = "";
		QFont font = pdfTextRegionLines[i].segments[0].pdfGlyphStyle.font;
		for (int glyphIndex = pdfTextRegionLines[i].glyphIndex; glyphIndex <= pdfTextRegionLines[i].segments[0].glyphIndex; glyphIndex++)
		{
			bodyText += glyphs[glyphIndex].code;
		}
		textNode->itemText.insertChars(bodyText);
		
		
		SlaOutputDev::applyTextStyle(textNode, pdfTextRegionLines[i].segments[0].pdfGlyphStyle.font.family(),
			pdfTextRegionLines[i].segments[0].pdfGlyphStyle.currColorFill,
			pdfTextRegionLines[i].segments[0].pdfGlyphStyle.font.pointSizeF(),
			pdfTextRegionLines[i].segments[0].pdfGlyphStyle.font.bold(),
			pdfTextRegionLines[i].segments[0].pdfGlyphStyle.font.italic(),
			pdfTextRegionLines[i].glyphIndex, (pdfTextRegionLines[i].segments[0].glyphIndex - pdfTextRegionLines[i].glyphIndex) + 1);
		
		for (int j = 1; j < (int)pdfTextRegionLines[i].segments.size(); j++)
		{
			bodyText = "";
			for (int glyphIndex = pdfTextRegionLines[i].segments[j - 1].glyphIndex + 1;
				glyphIndex <= pdfTextRegionLines[i].segments[j].glyphIndex; glyphIndex++)
			{
				QString number;
				bodyText += glyphs[glyphIndex].code;
			}
			textNode->itemText.insertChars(bodyText);
			SlaOutputDev::applyTextStyle(textNode, pdfTextRegionLines[i].segments[j].pdfGlyphStyle.font.family(),
				pdfTextRegionLines[i].segments[j].pdfGlyphStyle.currColorFill,
				pdfTextRegionLines[i].segments[j].pdfGlyphStyle.font.pointSizeF(),
				pdfTextRegionLines[i].segments[0].pdfGlyphStyle.font.bold(),
				pdfTextRegionLines[i].segments[0].pdfGlyphStyle.font.italic(),
				pdfTextRegionLines[i].segments[j - 1].glyphIndex + 1, (pdfTextRegionLines[i].segments[j].glyphIndex - pdfTextRegionLines[i].segments[j - 1].glyphIndex));
		}

	}
	textNode->frameTextEnd();
}

/*
*	Quick test to see if this is a virgin textregion
*/
bool PdfTextRegion::isNew()
{
	return pdfTextRegionLines.empty() ||
		glyphs.empty();
}

void PdfTextRegion::SetNewFontAndStyle(PdfGlyphStyle* fontStyle)
{
	m_newFontStyleToApply = fontStyle;
}


PdfTextOutputDev::PdfTextOutputDev(ScribusDoc* doc, QList<PageItem*>* Elements, QStringList* importedColors, int flags) : SlaOutputDev(doc, Elements, importedColors, flags)
{
	QFontDatabase fontDatabase;
	m_availableFontNames = fontDatabase.families();
}

PdfTextOutputDev::~PdfTextOutputDev()
{
	// Nothing to do at the moment
}


/**.
 * \updates the text scaling and rotation matrix when called from poppler
 */
void PdfTextOutputDev::updateTextMat(GfxState* state)
{
	//qDebug() << "updateTextMat()";
	const double* text_matrix = state->getTextMat();
	double w_scale = sqrt(text_matrix[0] * text_matrix[0] + text_matrix[2] * text_matrix[2]);
	double h_scale = sqrt(text_matrix[1] * text_matrix[1] + text_matrix[3] * text_matrix[3]);
	double max_scale;
	if (w_scale > h_scale)
	{
		max_scale = w_scale;
	}
	else
	{
		max_scale = h_scale;
	}
	// Calculate new text matrix
	QTransform new_text_matrix = QTransform(text_matrix[0] * state->getHorizScaling(),
		text_matrix[1] * state->getHorizScaling(),
		-text_matrix[2], -text_matrix[3],
		0.0, 0.0);

	if (!qFuzzyCompare(fabs(max_scale), 1.0))
	{
		// Cancel out scaling by font size in text matrix
		new_text_matrix.scale(1.0 / max_scale, 1.0 / max_scale);
	}
	if (m_textMatrix != new_text_matrix || m_fontScaling != max_scale)
	{
		m_textMatrix = new_text_matrix;
		m_fontScaling = max_scale;
#ifdef DEBUG_TEXT_IMPORT_FONTS
		qDebug() << "Rescaling the font, " << max_scale << " this should no longer need to call or trigger update font inseted it should set the correct addchar mode";
#endif
		updateFont(state);
	}
}

/*
 *	Updates current text position and move to a position and or add a new glyph at the previous position.
 *	NOTE: The success == TextRegion::LineType::FAIL test is an invariant test that should never pass. if a rogue glyph is detected then it means there is a bug in the logic probably in TextRegion::addGlyphAtPoint or TextRegion::linearTest or TextRegion::moveToPoint
 *	FIXME: render the textframe, this should be done after the document has finished loading the current page so all the layout fix-ups can be put in-place first
 *	FIXME: textRegion needs to support moveBackOneGlyph instead of my manual implementation in this function.
 */
void PdfTextOutputDev::updateTextPos(GfxState* state)
{
	QPointF newPosition = QPointF(state->getCurX(), state->getCurY());
	PdfTextRegion* activePdfTextRegion = &m_pdfTextRecognition.activePdfTextRegion;

	if (activePdfTextRegion->isNew())
	{
		activePdfTextRegion->pdfTextRegionBasenOrigin = newPosition;		
		m_pdfTextRecognition.setCharMode(PdfTextRecognition::AddCharMode::ADDCHARWITHBASESTLYE);
	}
	else
	{
		// if we've will move to a new line or new text region then update the current text region with the last glyph, this ensures all textlines and textregions have terminating glyphs.
		if (m_pdfTextRecognition.isNewLineOrRegion(newPosition))
		{
			QPointF glyphPosition = activePdfTextRegion->lastXY;
			activePdfTextRegion->lastXY.setX(activePdfTextRegion->lastXY.x() - activePdfTextRegion->glyphs.back().dx);
			PdfGlyph pdfGlyph = activePdfTextRegion->glyphs.back();
			activePdfTextRegion->SetNewFontAndStyle(&this->m_previouisGlyphStyle);
			if (activePdfTextRegion->addGlyphAtPoint(glyphPosition, activePdfTextRegion->glyphs.back()) == PdfTextRegion::LineType::FAIL)
				qDebug("FIXME: Rogue glyph detected, this should never happen because the cursor should move before glyphs in new regions are added.");
#ifdef DEBUG_TEXT_IMPORT
			else
				qDebug() << "Newline should be next";
#endif
			activePdfTextRegion->lastXY.setX(glyphPosition.x());
		}
	}
	PdfTextRegion::LineType linePdfTestResult = activePdfTextRegion->moveToPoint(newPosition);
	if(linePdfTestResult == PdfTextRegion::LineType::STYLENORMALRETURN)
	{
		this->m_pdfTextRecognition.setCharMode(PdfTextRecognition::AddCharMode::ADDCHARWITHBASESTLYE);
	} 
	else if (linePdfTestResult == PdfTextRegion::LineType::SAMELINE || linePdfTestResult== PdfTextRegion::LineType::STYLESUPERSCRIPT)
	{
		m_pdfTextRecognition.setCharMode(PdfTextRecognition::AddCharMode::ADDCHARWITHBASESTLYE);
	}
	else if (linePdfTestResult == PdfTextRegion::LineType::FAIL)
	{
#ifdef DEBUG_TEXT_IMPORT
		qDebug("updateTextPos: renderPdfTextFrame() + m_pdfTextRecognition.addPdfTextRegion()");
#endif
		if(m_pdfTextRecognition.activePdfTextRegion.pdfTextRegionLines.size() > 1)
			m_pdfTextRecognition.doBreaksAnbdSpaces();
		renderTextFrame();
		m_pdfTextRecognition.addPdfTextRegion();
		updateTextPos(state);
	}
}

/*
*	render the textregion to a new PageItem::TextFrame, currently some hackjish defaults have been implemented there are a number of FIXMEs and TODOs
*	FIXME: Paragraphs need to be implemented properly  this needs to be applied to the charstyle of the default pstyle
*	FIXME xcord and ycord need to be set properly based on GfxState and the page transformation matrix
*	TODO: Implement paragraph styles
*	TODO Decide if we should be setting the clipshape of the POoLine values as is the case with other import implementations
*/
void PdfTextOutputDev::renderTextFrame()
{
	//qDebug() << "_flushText()    m_doc->currentPage()->xOffset():" << m_doc->currentPage()->xOffset();
	auto activePdfTextRegion = &m_pdfTextRecognition.activePdfTextRegion;
	if (activePdfTextRegion->glyphs.empty())
		return;

	qreal xCoor = m_doc->currentPage()->xOffset() + activePdfTextRegion->pdfTextRegionBasenOrigin.x();
	qreal yCoor = m_doc->currentPage()->initialHeight() - (m_doc->currentPage()->yOffset() + (double)activePdfTextRegion->pdfTextRegionBasenOrigin.y() + activePdfTextRegion->lineSpacing); // don't know if y is top down or bottom up
	qreal  lineWidth = 0.0;
#ifdef DEBUG_TEXT_IMPORT
	qDebug() << "rendering new frame at:" << xCoor << "," << yCoor << " With lineheight of: " << activePdfTextRegion->lineSpacing << "Height:" << activePdfTextRegion->maxHeight << " Width:" << activePdfTextRegion->maxWidth;
#endif
	int z = m_doc->itemAdd(PageItem::TextFrame, PageItem::Rectangle, xCoor, yCoor, 40, 40, 0, CommonStrings::None, CommonStrings::None);
	PageItem* textNode = m_doc->Items->at(z);

	ParagraphStyle& pStyle = (ParagraphStyle&)textNode->itemText.defaultStyle();
	pStyle.setLineSpacingMode(pStyle.AutomaticLineSpacing);
	pStyle.setHyphenationMode(pStyle.AutomaticHyphenation);
	finishItem(textNode);
	textNode->ClipEdited = true;
	textNode->FrameType = 3;
	textNode->setLineEnd(PLineEnd);
	textNode->setLineJoin(PLineJoin);
	textNode->setTextFlowMode(PageItem::TextFlowDisabled);
	textNode->setLineTransparency(1.0);
	textNode->setFillColor(CommonStrings::None);
	textNode->setLineColor(CommonStrings::None);
	textNode->setLineWidth(0);
	textNode->setFillShade(CurrFillShade);


	/* Oliver Stieber 2020-06-11 Set text matrix... This need to be done so that the global world view that we rite out glyphs to is transformed correctly by the context matrix for each glyph, possibly anyhow.
	needs the way in which we are handling transformations for the page to be more concrete before this code can be implemented either here or somewhere else
	FIXME: Setting the text matrix isn't supported at the moment
	QTransform text_transform(_text_matrix);
	text_transform.setMatrix(text_transform.m11(), text_transform.m12(), 0,
		text_transform.m21(), text_transform.m22(), 0,
		first_glyph.position.x(), first_glyph.position.y(), 1);
	gchar *transform = sp_svg_transform_write(text_transform);
	text_node->setAttribute("transform", transform);
	g_free(transform);
	*/

	int shade = 100;
	/*
	* This code sets the font and style in a very simplistic way, it's been commented out as it needs to be updated to be used within PdfTextRecognition &co.
	*/
	QString CurrColorText = CurrColorFill;//CurrFillShade;// getColor(state->getFillColorSpace(), state->getFillColor(), &shade);
	/*
	CharStyle& cStyle = static_cast<CharStyle&>(pStyle.charStyle());
	cStyle.setScaleH(1000.0);
	cStyle.setScaleV(1000.0);
	cStyle.setHyphenChar(SpecialChars::BLANK.unicode());

	textNode->itemText.setDefaultStyle(pStyle);
	textNode->invalid = true;
	*/
	activePdfTextRegion->renderToTextFrame(textNode);
	textNode->itemText.insertChars(SpecialChars::PARSEP, true);

	/*
	*	This code can be used to set PoLine instead of setting the FrameShape if setting the PoLine is the more correct way of doing things.
	*	I have no idea of what the PoLine is at this time except for it changes when the shape is set and appears to be unit scales as opposed to percentage scaled
	FPointArray boundingBoxShape;
	boundingBoxShape.resize(0);
	boundingBoxShape.svgInit();
	//doubles to create a shape, it's 100% textframe width by 100% textframe height

	boundingBoxShape.svgMoveTo(TextRegion::boundingBoxShape[0], TextRegion::boundingBoxShape[1]);
	for (int a = 0; a < 16; a += 2)
	{
		boundingBoxShape.append(FPoint(TextRegion::boundingBoxShape[a * 2], TextRegion::boundingBoxShape[a * 2 + 1]));
	}
	boundingBoxShape.scale(textNode->width() / 100.0, textNode->height() / 100.0);
	*/
	textNode->SetFrameShape(32, PdfTextRegion::boundingBoxShape);
	textNode->ContourLine = textNode->PoLine.copy();

	m_doc->Items->removeLast();
	m_Elements->append(textNode);
	if (m_groupStack.count() != 0)
	{
		m_groupStack.top().Items.append(textNode);
		applyMask(textNode);
	}
}

/*
*	code mostly taken from importodg.cpp which also supports some line styles and more fill options etc...
*/
void PdfTextOutputDev::finishItem(PageItem* item)
{
	item->ClipEdited = true;
	item->FrameType = 3;
	/*code can be enabled when PoLine is set or when the shape is set as that sets PoLine
	FPoint wh = getMaxClipF(&item->PoLine);
	item->setWidthHeight(wh.x(), wh.y());
	item->Clip = flattenPath(item->PoLine, item->Segments);
	*/
	item->OldB2 = item->width();
	item->OldH2 = item->height();
	item->updateClip();
	item->OwnPage = m_doc->OnPage(item);
}

void PdfTextOutputDev::drawChar(GfxState* state, double x, double y, double dx, double dy, double originX, double originY, CharCode code, int nBytes, POPPLER_CONST_082 Unicode* u, int uLen)
{
	//qDebug() << "drawChar:" << (QChar)code;
	// TODO Implement the clipping operations. At least the characters are shown.
	int textRenderingMode = state->getRender();
	// Invisible or only used for clipping
	if (textRenderingMode == 3)
		return;
	if (textRenderingMode < 8)
	{
		m_pdfTextRecognition.addChar(state, x, y, dx, dy, originX, originY, code, nBytes, u, uLen);
	}
}

void PdfTextOutputDev::beginTextObject(GfxState* state)
{
	pushGroup();
	if (!m_pdfTextRecognition.activePdfTextRegion.pdfTextRegionLines.empty())
	{
#ifdef DEBUG_TEXT_IMPORT
		qDebug("beginTextObject: m_textRecognition.addTextRegion()");
#endif
		m_pdfTextRecognition.addPdfTextRegion();
	}
}

void PdfTextOutputDev::endTextObject(GfxState* state)
{
	if (!m_pdfTextRecognition.activePdfTextRegion.pdfTextRegionLines.empty())
	{
		// Add the last glyph to the textregion
		QPointF glyphXY = m_pdfTextRecognition.activePdfTextRegion.lastXY;
		m_pdfTextRecognition.activePdfTextRegion.lastXY.setX(m_pdfTextRecognition.activePdfTextRegion.lastXY.x() - m_pdfTextRecognition.activePdfTextRegion.glyphs.back().dx);
		if (m_pdfTextRecognition.activePdfTextRegion.addGlyphAtPoint(glyphXY, m_pdfTextRecognition.activePdfTextRegion.glyphs.back()) == PdfTextRegion::LineType::FAIL)
		{
			qDebug("FIXME: Rogue glyph detected, this should never happen because the cursor should move before glyphs in new regions are added.");
		}
#ifdef DEBUG_TEXT_IMPORT
		qDebug("endTextObject: renderTextFrame");
#endif
		if (m_pdfTextRecognition.activePdfTextRegion.pdfTextRegionLines.size() > 1)
			m_pdfTextRecognition.doBreaksAnbdSpaces();
		renderTextFrame();
	}
	else if (!m_pdfTextRecognition.activePdfTextRegion.pdfTextRegionLines.empty())
		qDebug("FIXME:Rogue textblock");

	m_pdfTextRecognition.setCharMode(PdfTextRecognition::AddCharMode::ADDCHARWITHBASESTLYE);

	SlaOutputDev::endTextObject(state);
}

/*
* update the font for the next block of glyphs.
* just a stub for now
*/
static void DumpFont(GfxFont*font)
{
#ifdef DEBUG_TEXT_IMPORT_FONTS
	qDebug() << "Condensed:" << font->Condensed << "Expanded:" << font->Expanded << "ExtraCondensed:" <<
		font->ExtraCondensed << "ExtraExpanded:" << font->ExtraExpanded << "getAlternateName:" << font->getAlternateName(font->getName()->c_str()) << "getAscent:" << font->getAscent();
	qDebug() << "getDescent:" << font->getDescent() << "getEmbeddedFontName:" << font->getEmbeddedFontName()->c_str() << "getEncodingName:" << font->getEncodingName()->c_str() << "getFamily:" <<
		(font->getFamily() != nullptr ? font->getFamily()->c_str() : "nullptr") << "getName:" << font->getName()->c_str() << "getStretch:" << font->getStretch() << "getType:" << font->getType() << "getWeight:" << font->getWeight();
	qDebug() << "isBold:" << font->isBold() << "isCIDFont:" << font->isCIDFont() << "isFixedWidth:" << font->isFixedWidth() << "isItalic:" << font->isItalic();
	qDebug() << "isOk:" << font->isOk() << "isSerif:" << font->isSerif() << "isSymbolic:" << font->isSymbolic();
	qDebug() << "WeightNotDefined:" << font->WeightNotDefined;
	qDebug() << "StretchNotDefined:" << font->StretchNotDefined;
	qDebug() << "Normal:" << font->Normal;
#endif
}

/**
 * \brief Updates _font_style according to the font set in parameter state
 FIXME: This function is rather large and unruly and it does lots of things so it's a good target for reducing down into simpler parts which would also make it easier to understand.
 TODO: Have a font name substitution cache, so we can just match up font->getName()->getCString() if it's already been processed
 */
void PdfTextOutputDev::updateFont(GfxState* state)
{
	//qDebug() << "updateFontForText()"
	m_needFontUpdate = false;
	if (m_pdfTextRecognition.activePdfTextRegion.isNew())
		m_lastFontSpecification = "Were in a new region";
	m_previouisGlyphStyle = m_pdfGlyphStyle;
	QFont origional_font_style = m_pdfGlyphStyle.font;
#ifdef DEBUG_TEXT_IMPORT_FONTS
	qDebug() << "origional_font_style:" << origional_font_style.toString();
	qDebug() << "m_lastFontSpecification:" << m_lastFontSpecification;
#endif

	GfxFont* font = state->getFont();
	// Store original name	
	QString _fontSpecification;
	if (font != NULL && font->getName())
	{

		auto newFontSDpecification = static_cast<QString>(font->getName()->c_str());		 
		if (m_lastFontSpecification != newFontSDpecification)
		{
			m_lastFontSpecification = newFontSDpecification;
			_fontSpecification = newFontSDpecification;
		}
	}
	else
	{
		auto newFontSDpecification = static_cast<QString>("Arial");
		if (m_lastFontSpecification != newFontSDpecification)
		{
			 m_lastFontSpecification = newFontSDpecification;
			 _fontSpecification = newFontSDpecification;
		}
	}

	m_pdfTextRecognition.setCharMode(PdfTextRecognition::AddCharMode::ADDCHARWITHNEWSTYLE);
	m_previouisGlyphStyle = m_pdfGlyphStyle;
	
	//we've got this far so we are good to start afresh with a new PdfGlyphStyle
	m_pdfGlyphStyle.font = QFont();
	// Prune the font name to get the correct font family name
	// In a PDF font names can look like this: IONIPB+MetaPlusBold-Italic
	QString font_family;
	QString font_style;
	QString font_style_lowercase;
	QString plus_sign = _fontSpecification.mid(_fontSpecification.indexOf("+") + 1);
	if (plus_sign.length() > 0)
	{
		font_family = QString(plus_sign);
		_fontSpecification = plus_sign;
	}
	else
		font_family = _fontSpecification;


	int style_delim = 0;
	style_delim = font_family.indexOf("-");
	if (style_delim < 0)
		style_delim = font_family.indexOf(",");
	if (style_delim >= 0)
	{		
		font_style = font_family.mid(style_delim + 1);
		font_style_lowercase = font_style.toLower();
		font_family = font_family.left(style_delim);
	}

	QString cs_font_family = "Arial";
	// Font family
	if (font != NULL && font->getFamily())
		cs_font_family = font->getFamily()->getCString();
	else
	{
		int attr_value = 1;
		if (attr_value != 0)
		{
			// Find the font that best matches the stripped down (orig)name (Bug LP #179589).
			QString fontMatchingString = font_family;
			if (font_style.length() > 0)
				fontMatchingString += "-" + font_style;
			cs_font_family = BestMatchingFont(fontMatchingString);
		}
		else
		{
			cs_font_family = font_family;
		}
	}
	
	m_pdfGlyphStyle.font.setFamily(cs_font_family);
	m_pdfGlyphStyle.font.setStyle(QFont::StyleNormal);
	// Font style
	if (font != NULL && font->isItalic())
		m_pdfGlyphStyle.font.setStyle(QFont::StyleItalic);
	else
		if (font_style != "")
			if ((font_style_lowercase.indexOf("italic") >= 0) ||
				font_style_lowercase.indexOf("slanted") >= 0)
				m_pdfGlyphStyle.font.setStyle(QFont::StyleItalic);
			else if (font_style_lowercase.indexOf("oblique") >= 0)
				m_pdfGlyphStyle.font.setStyle(QFont::StyleOblique);
#ifdef DEBUG_TEXT_IMPORT_FONTS
	qDebug() << "font_style_lowercase:" << font_style_lowercase;
#endif
	if (font != NULL && font->isBold())
		m_pdfGlyphStyle.font.setBold(true);
	else if ((font_style_lowercase.indexOf("bold") >= 0) ||
		font_style_lowercase.indexOf("black") >= 0)
		m_pdfGlyphStyle.font.setBold(true);

	// Font weight
	GfxFont::Weight font_weight = font != NULL ? font->getWeight() : GfxFont::W400;	
	char* css_font_weight = nullptr;

	if (font_weight != GfxFont::WeightNotDefined)
		if (font_weight == GfxFont::W400)
			m_pdfGlyphStyle.font.setWeight(QFont::Normal);
		else if (font_weight == GfxFont::W700)
			m_pdfGlyphStyle.font.setWeight(QFont::Bold);

	m_pdfGlyphStyle.font.setStretch(QFont::Unstretched);
	// Font stretch
	GfxFont::Stretch font_stretch = font != NULL ? font->getStretch() : GfxFont::Normal;
	switch (font_stretch)
	{
	case GfxFont::UltraCondensed:
		m_pdfGlyphStyle.font.setStretch(QFont::UltraCondensed);
		break;
	case GfxFont::ExtraCondensed:
		m_pdfGlyphStyle.font.setStretch(QFont::ExtraCondensed);
		break;
	case GfxFont::Condensed:
		m_pdfGlyphStyle.font.setStretch(QFont::Condensed);
		break;
	case GfxFont::SemiCondensed:
		m_pdfGlyphStyle.font.setStretch(QFont::SemiCondensed);
		break;
	case GfxFont::Normal:
		m_pdfGlyphStyle.font.setStretch(QFont::Unstretched);
		break;
	case GfxFont::SemiExpanded:
		m_pdfGlyphStyle.font.setStretch(QFont::SemiExpanded);
		break;
	case GfxFont::Expanded:
		m_pdfGlyphStyle.font.setStretch(QFont::Expanded);
		break;
	case GfxFont::ExtraExpanded:
		m_pdfGlyphStyle.font.setStretch(QFont::ExtraExpanded);
		break;
	case GfxFont::UltraExpanded:
		m_pdfGlyphStyle.font.setStretch(QFont::UltraExpanded);
		break;
	default:
		break;
	}

	//I think font scaling should be handled outside this function like things like colour are. convert pixels to points 0.75
	double css_font_size = m_fontScaling * state->getFontSize() *0.75;
#ifdef DEBUG_TEXT_IMPORT_FONTS
	qDebug() << "m_fontScaling: " << m_fontScaling << "state->getFontSize():" << state->getFontSize();
#endif

	m_pdfGlyphStyle.font.setPointSizeF(css_font_size);

	m_pdfGlyphStyle.font.resolve();
#ifdef DEBUG_TEXT_IMPORT_FONTS
	if (m_pdfGlyphStyle.font.key() == origional_font_style.key() || m_pdfGlyphStyle.font.toString() == origional_font_style.toString())
	{

		qDebug() << "Font hasn't changed .. " << m_pdfGlyphStyle.font.key() << "  " << m_pdfGlyphStyle.font.toString() << endl;
	}
	else
	{
		qDebug() << "Font has changed to.. " << m_pdfGlyphStyle.font.key() << "  " << m_pdfGlyphStyle.font.toString() << endl;
		qDebug() << "Font has changed from.. " << origional_font_style.key() << "  " << origional_font_style.toString() << endl;

	}
#endif
	m_pdfTextRecognition.setFillColour(m_pdfGlyphStyle.currColorFill);
	m_pdfTextRecognition.setStrokeColour(m_pdfGlyphStyle.currColorStroke);	
	m_pdfTextRecognition.setPdfGlyphStyleFont(m_pdfGlyphStyle.font);
	m_pdfTextRecognition.setCharMode(PdfTextRecognition::AddCharMode::ADDCHARWITHBASESTLYE);
}

void PdfTextOutputDev::updateFillColor(GfxState* state)
{
	SlaOutputDev::updateFillColor(state);
	m_pdfGlyphStyle.currColorFill = CurrColorFill;
	m_pdfTextRecognition.setFillColour(CurrColorFill);
}

void PdfTextOutputDev::updateStrokeColor(GfxState* state) 
{
	SlaOutputDev::updateStrokeColor(state);
	m_pdfGlyphStyle.currColorStroke = CurrColorStroke;
	m_pdfTextRecognition.setStrokeColour(CurrColorStroke);
}

/*
* NOTE: Override these for now and do nothing so they don't get picked up and rendered as vectors by the base class,
	though in the long run we may actually want that unless they can be implemented in a similar way to the text import getChar in which case overloading the makes perfect sense.
*/
GBool PdfTextOutputDev::beginType3Char(GfxState* state, double x, double y, double dx, double dy, CharCode code, POPPLER_CONST_082 Unicode* u, int uLen)
{
	//stub
	return gFalse;
}
void  PdfTextOutputDev::endType3Char(GfxState* state)
{
	//stub
}
void  PdfTextOutputDev::type3D0(GfxState* state, double wx, double wy)
{
	//stub
}
void  PdfTextOutputDev::type3D1(GfxState* state, double wx, double wy, double ll, double lly, double urx, double ury)
{
	//stub
}

/**
 * \brief Shifts the current text position by the given amount (specified in text space)
 */
void PdfTextOutputDev::updateTextShift(GfxState* state, double shift)
{
//	qDebug() << "updateTextShift() shift:" << shift;
}


/*
	MatchingChars
	Count for how many characters s1 matches sp taking into account
	that a space in sp may be removed or replaced by some other tokens
	specified in the code. (Bug LP #179589)
*/
size_t PdfTextOutputDev::MatchingChars(QString s1, QString sp)
{
	int is = 0;
	int ip = 0;

	while (is < s1.length() && ip < sp.length())
	{
		if (s1[is] == sp[ip])
		{
			is++; ip++;
		}
		else if (sp[ip] == ' ')
		{
			ip++;
			if (s1[is] == '_')
			{ // Valid matches to spaces in sp.
				is++;
			}
		}
		else
		{
			break;
		}
	}
	return ip;
}

static int CountMatchingStringListItems(QStringList& listA, QStringList& listB)
{
	int matchCount = 0;
	for (auto listelementA : listA)
	{
		for (auto listelementB : listB)
		{
			if (listelementA.toLower() == listelementB.toLower())
			{
				matchCount++;
			}
		}
	}
	return matchCount;
}

/*
	SvgBuilder::BestMatchingFont
	Scan the available fonts to find the font name that best matches PDFname.
*/
QString PdfTextOutputDev::BestMatchingFont(QString PDFname)
{
	//qDebug() << "_bestMatchingFont():" << PDFname;
	double bestMatch = 0;
	QString bestFontname = "Arial";

	for (auto fontname : m_availableFontNames)
	{
		QStringList familyNameAndCharacteristicsAF = fontname.split(" ");
		QStringList familyNameAndCharacteristicsPDF = PDFname.split("-");  //also Camel case sometimes.
		size_t matchingItems = CountMatchingStringListItems(familyNameAndCharacteristicsAF, familyNameAndCharacteristicsPDF);
		if (matchingItems > bestMatch)
		{
			bestMatch = matchingItems;
			bestFontname = fontname;
		}
	}

	if (bestMatch > 0)
		return bestFontname;
	else
		return "Arial";
}

