#include <hogboxHUD/TextRegion.h>

#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <hogbox/AssetManager.h>
#include <osgDB/ReadFile>

using namespace hogboxHUD;

#ifndef WIN32
#define SHADER_COMPAT \
"#ifndef GL_ES\n" \
"#if (__VERSION__ <= 110)\n" \
"#define lowp\n" \
"#define mediump\n" \
"#define highp\n" \
"#endif\n" \
"#endif\n" 
#else
#define SHADER_COMPAT ""
#endif

static const char* textVertSource = { 
	SHADER_COMPAT 
	"attribute vec4 osg_Vertex;\n"
	"attribute vec4 osg_MultiTexCoord0;\n"
    "attribute vec4 osg_Color;\n"
	"uniform mat4 osg_ModelViewProjectionMatrix;\n"
	"varying mediump vec2 texCoord0;\n"
    "varying mediump vec4 color;\n"
	"void main(void) {\n" 
	"  gl_Position = osg_ModelViewProjectionMatrix * osg_Vertex;\n" 
	"  texCoord0 = osg_MultiTexCoord0.xy;\n"
    "  color = osg_Color;\n"
	"}\n" 
}; 

static const char* textFragSource = { 
	SHADER_COMPAT 
	"uniform sampler2D glyphTexture;\n"
    "//uniform mediump vec4 osg_Color;\n"
	"varying mediump vec2 texCoord0;\n"
    "varying mediump vec4 color;\n"
	"void main(void) {\n" 
	"  gl_FragColor.rgb = color.rgb;\n"
    "  gl_FragColor.a = texture2D(glyphTexture, texCoord0).a * color.a;\n" 
	"}\n" 
};


TextRegion::TextRegion(RegionPlane plane, RegionOrigin origin, bool isProcedural) 
    : HudRegion(plane, origin, isProcedural),
    _string(""),
    _text(NULL),
    _fontHeight(18.0f),
    _fontName("Fonts/arial.ttf"),
    _boarderPadding(5.0f),
    _alignmentMode(CENTER_ALIGN),
    _textColor(osg::Vec4(0.1f,0.1f,0.1f,1.0f)),
    _backdropType(NO_BACKDROP),
    _backdropColor(osg::Vec4(0.1f,0.1f,0.1f,0.7f)),
    //callback events
    _onTextChangedEvent(new CallbackEvent(this, "OnTextChanged"))

{
	//create the text label to add to the button
	_text = new osgText::Text;
	//_text->setCharacterSizeMode(osgText::TextBase::OBJECT_COORDS_WITH_MAXIMU_SCREEN_SIZE_CAPPED_BY_FONT_HEIGHT);
    
	//add the text to a geode for drawing
	osg::Geode* textGeode = new osg::Geode();
    //geode is visible, not pickable
    textGeode->setNodeMask(hogbox::MAIN_CAMERA_CULL);
    osg::StateSet* stateset = textGeode->getOrCreateStateSet();
	stateset->setMode(GL_LIGHTING,osg::StateAttribute::OFF);
    
#ifndef OSG_GL_FIXED_FUNCTION_AVAILABLE
	osg::Program* program = new osg::Program; 
	program->setName("textShader"); 
	program->addShader(new osg::Shader(osg::Shader::VERTEX, textVertSource)); 
	program->addShader(new osg::Shader(osg::Shader::FRAGMENT, textFragSource)); 
	stateset->setAttributeAndModes(program, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE); 	
    
    stateset->addUniform(new osg::Uniform("glyphTexture", 0));
#endif
    
	textGeode->setStateSet(stateset);
    textGeode->addDrawable(_text.get());
    
    _textScale = new osg::MatrixTransform();
    _textScale->addChild(textGeode);
    
	//use transform to orientate the text into correct plane and forward of the backdrop
	osg::MatrixTransform* oriText = new osg::MatrixTransform();
    
    //flip plane
    float temp = 0.0f;
    osg::Vec3 offset = osg::Vec3(0,0,0.1);
    osg::Matrix oriMatrix;
    switch(_plane){
        case PLANE_XY:
            oriMatrix = osg::Matrix::translate(offset);
            break;
        case PLANE_XZ:
            //flip x and z
            temp = offset.y();
            offset.y() = offset.z();
            offset.z() = temp;
            oriMatrix = osg::Matrix::translate(offset) * osg::Matrix::rotate(osg::DegreesToRadians(90.0f), osg::Vec3(1,0,0));
            break;
        default:break;
    }
    
	oriText->setMatrix(oriMatrix);
	oriText->addChild(_textScale.get());
	//attach the text to translate for now 
	_rotate->addChild(oriText);
    
    _text->setColor(_textColor);
	this->SetFontType(_fontName);
	this->SetFontHeight(_fontHeight);
	this->SetAlignment(_alignmentMode);
	this->SetBackDropType(_backdropType);
	this->SetBackDropColor(_backdropColor);
}

/** Copy constructor using CopyOp to manage deep vs shallow copy.*/
TextRegion::TextRegion(const TextRegion& region,const osg::CopyOp& copyop)
    : HudRegion(region, copyop),
    //_text(copyop(region._text.get())),
    _fontHeight(region._fontHeight),
    _fontName(region._fontName),
    _boarderPadding(region._boarderPadding),
    _alignmentMode(region._alignmentMode),
    _textColor(region._textColor),
    _backdropType(region._backdropType),
    _backdropColor(region._backdropColor)
{
	//this->SetTextColor(_textColor);
	this->SetFontType(_fontName);
	this->SetFontHeight(_fontHeight);
	this->SetAlignment(_alignmentMode);
	this->SetBackDropType(_backdropType);
	this->SetBackDropColor(_backdropColor);
}

TextRegion::~TextRegion(void)
{
	//OSG_NOTICE << "    Deallocating TextRegion: Name '" << this->getName() << "'." << std::endl;
	_onTextChangedEvent = NULL;
    _text = NULL;
}


//
// Create a text region using a fileName as the background
// then creating an osg text geode and attaching directly to translate
// so that the texts size can be set directly in pixels, without
//being scaled by the _scale matrix
//
bool TextRegion::Create(osg::Vec2 corner, osg::Vec2 size, const std::string& fileName, const std::string& label, float fontHeight)
{
    //set ref height for scaling in setsize
    _scaleHeightRef = size.y();
    
	//load the base assets and apply names and sizes, do this
	//after creating and adding the text so that the text geode 
	//name will also be changed
	bool ret = HudRegion::Create(corner,size,fileName);
    
	SetText(label);
    SetFontHeight(fontHeight);
	
	return ret;
}

//
//Text region loads the aditional assests
//searches for a ttf file to use as a font
//
bool TextRegion::LoadAssest(const std::string& folderName)
{
	//call base first
	bool ret = HudRegion::LoadAssest(folderName);
    
	//get the directory contents
	osgDB::DirectoryContents dirCont;
	dirCont = osgDB::getDirectoryContents(folderName);
	//loop all the files and look for ttf extension
	for(unsigned int i=0; i<dirCont.size(); i++)
	{
		if(osgDB::getFileExtension(dirCont[i]).compare("ttf") == 0)
		{
			//try to load as font
			this->SetFontType(dirCont[i]);
		}
	}
	return ret;
}

//
//overload set position
//
void TextRegion::SetPosition(const osg::Vec2& corner)
{
	HudRegion::SetPosition(corner);
}

//overload SetSize so we can set the texts pixelheight independantly
void TextRegion::SetSize(const osg::Vec2& size)
{
	HudRegion::SetSize(size);
    
	//set the texts max sizes to the new size
	_text->setMaximumHeight(size.y());
	_text->setMaximumWidth(size.x());
    
    float scaler = 1.0f / _scaleHeightRef;
    float sizeScale = size.y()*scaler;
    
    _textScale->setMatrix(osg::Matrix::scale(osg::Vec3(sizeScale,sizeScale,sizeScale)));
    
	//set new centered position
	this->SetAlignment(_alignmentMode);
}

//
// Set the text for this region
//
void TextRegion::SetText(const std::string& str)
{
	_string = str;
	_text->setText(str);
	osg::ref_ptr<HudInputEvent> dummyEvent;
	_onTextChangedEvent->TriggerEvent(*dummyEvent.get());
}

//
//return the current text vaule
//
const std::string& TextRegion::GetText()const
{
	return _string;
}


//
//set font height in pixels
//
void TextRegion::SetFontHeight(const float& fontHeight)
{
	_fontHeight = fontHeight;
	_text->setCharacterSize(_fontHeight, 1.0f);
}

//
//Return current font height
//
const float& TextRegion::GetFontHeight()const
{
	return _fontHeight;
}

//
//set the fonts glyph res in pixels
//
void TextRegion::SetFontResolution(const osg::Vec2& fontRes)
{
    _text->setFontResolution((int)fontRes.x(), (int)fontRes.y());
}

//
//set the texts font type
//
void TextRegion::SetFontType(const std::string& fontFile)
{
	_fontName = osgDB::findDataFile(fontFile);
    osgText::FontPtr font = AssetManager::Inst()->getOrLoadFont(fontFile).get();
    
    if(font.geT())
        _text->setFont(_fontName);
    }
}

//
//Return the name of the font type
//being used
//
const std::string& TextRegion::GetFontType()const
{
	return _fontName;
}

//
//set the number of pixels between
//the edge of the text and the edge of the
//region when in left or right alignments
//
const float& TextRegion::GetBoarderPadding()const
{
	return _boarderPadding;
}

void TextRegion::SetBoarderPadding(const float& padding)
{
	_boarderPadding = padding;
}

//
//Set the text alignment mode
//
void TextRegion::SetAlignment(const TEXT_ALIGN& alignment)
{
	_alignmentMode = alignment;
    
    osg::Vec3 offset = osg::Vec3(0.0f,0.0f,0.0f);
    float temp = 0.0f;
	switch(alignment)
	{
		case CENTER_ALIGN:
		{
            offset = osg::Vec3(_size.x()*0.5f, _size.y()*0.5f, 0.0f);
            switch(_origin){
                case ORI_BOTTOM_LEFT:
                    break;
                case ORI_TOP_LEFT:
                    offset.y() *= -1.0f;
                    break;
                case ORI_CENTER:
                    offset = osg::Vec3(0.0f,0.0f,0.0f);
                    break;
                default:break;
            }
            //flip plane
            switch(_plane){
                case PLANE_XY:
                    break;
                case PLANE_XZ:
                    //flip x and z
                    temp = offset.y();
                    offset.y() = offset.z();
                    offset.z() = temp;
                    break;
                default:break;
            }
            
			_text->setAlignment(osgText::Text::CENTER_CENTER ); 
			_text->setPosition(offset);
			break;
		}
		case RIGHT_ALIGN:
		{
            offset = osg::Vec3(_size.x()-_boarderPadding, _size.y()*0.5f, 0);
            switch(_origin){
                case ORI_BOTTOM_LEFT:
                    break;
                case ORI_TOP_LEFT:
                    offset.y() *= -1.0f;
                    break;
                case ORI_CENTER:
                    offset += osg::Vec3((_size.x()*0.5f)-_boarderPadding,0.0f,0.0f);
                    break;
                default:break;
            }
            //flip plane
            switch(_plane){
                case PLANE_XY:
                    break;
                case PLANE_XZ:
                    //flip x and z
                    temp = offset.y();
                    offset.y() = offset.z();
                    offset.z() = temp;
                    break;
                default:break;
            }
            
			_text->setAlignment(osgText::Text::RIGHT_CENTER); 
			_text->setPosition(offset);
			break;
		}
		case LEFT_ALIGN:
		{
            offset = osg::Vec3(_boarderPadding, _size.y()*0.5f, 0);
            switch(_origin){
                case ORI_BOTTOM_LEFT:
                    break;
                case ORI_TOP_LEFT:
                    offset.y() *= -1.0f;
                    break;
                case ORI_CENTER:
                    offset += osg::Vec3((-_size.x()*0.5f)+_boarderPadding,0.0f,0.0f);
                    break;
                default:break;
            }
            //flip plane
            switch(_plane){
                case PLANE_XY:
                    break;
                case PLANE_XZ:
                    //flip x and z
                    temp = offset.y();
                    offset.y() = offset.z();
                    offset.z() = temp;
                    break;
                default:break;
            }
            
			_text->setAlignment(osgText::Text::LEFT_CENTER); 
			_text->setPosition(offset);
			break;
		}
		default:
		{
			_text->setAlignment(osgText::Text::CENTER_CENTER ); 
			break;
		}
	}
}

//
//Return the text alignment mode
//
const TextRegion::TEXT_ALIGN& TextRegion::GetAlignment()const
{
	return _alignmentMode;
}

//
//overload set alpha so we can also adust text color alpha
//
void TextRegion::SetAlpha(const float& alpha)
{
	_textColor.a() = alpha;
    _backdropColor.a() = alpha*0.5f;
	if(!_alphaEnabled)
	{
		_textColor.a() = 1.0f;
        _backdropColor.a() = 1.0f;
	}
	
    _text->setBackdropColor(_backdropColor);
	_text->setColor(_textColor);
	HudRegion::SetAlpha(alpha);
}

//
//Set the color of the text
//
void TextRegion::SetTextColor(const osg::Vec4& color)
{
	_textColor.r() = color.x();
	_textColor.g() = color.y();
	_textColor.b() = color.z();
    _textColor.a() = _alpha;
	_text->setColor(_textColor);
    this->SetColor(osg::Vec3(color.x(),color.y(),color.z()));
}

//
//Return the color of the text
//
const osg::Vec4& TextRegion::GetTextColor()const
{
	return _text->getColor();
}

//
//set the drop shadow color
//
void TextRegion::SetBackDropColor(const osg::Vec4 &color)
{
	_backdropColor.r() = color.r();
	_backdropColor.g() = color.g();
	_backdropColor.b() = color.b();
	_backdropColor.a() = _alpha*0.5f;
	_text->setBackdropColor(_backdropColor);
}

//
//return color of drop show
//
const osg::Vec4& TextRegion::GetBackDropColor()const
{
	return _backdropColor;
}

//
//enable/disable the texts drop shadow/stroke
//
void TextRegion::SetBackDropType(const BACKDROP_TYPE& type)
{
	_backdropType = type;
    
	if(_backdropType == DROP_SHADOW)
	{
		_text->setBackdropType(osgText::Text::DROP_SHADOW_BOTTOM_LEFT);
		_text->setBackdropImplementation(osgText::Text::DELAYED_DEPTH_WRITES );
	}else if(_backdropType == STROKE){
        _text->setBackdropType(osgText::Text::OUTLINE);
		_text->setBackdropImplementation(osgText::Text::DELAYED_DEPTH_WRITES );
    }else{ 
        _text->setBackdropType(osgText::Text::NONE);
	}
}

const TextRegion::BACKDROP_TYPE& TextRegion::GetBackDropType() const
{
	return _backdropType;
}

//
//bit hacky but get/set the backdrop type using int
//to conform to xml serialising requirements
//
void TextRegion::SetBackDropTypeInt(const int& type)
{
    this->SetBackDropType((BACKDROP_TYPE)(type));
}
const int& TextRegion::GetBackDropTypeInt() const
{
    return _backdropType;
}

int TextRegion::HandleInputEvent(HudInputEvent& hudEvent)
{
	return HudRegion::HandleInputEvent(hudEvent); 
}

//
//Funcs to register event callbacks
void TextRegion::AddOnTextChangedCallbackReceiver(HudEventCallback* callback)
{
	_onTextChangedEvent->AddCallbackReceiver(callback);
}