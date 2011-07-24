// Copyright (c) 2010-2011 Zipline Games, Inc. All Rights Reserved.
// http://getmoai.com

#include "pch.h"
#include <moaicore/MOAIDataBuffer.h>
#include <moaicore/MOAIGfxDevice.h>
#include <moaicore/MOAIImage.h>
#include <moaicore/MOAILogMessages.h>
#include <moaicore/MOAIPvrHeader.h>
#include <moaicore/MOAITexture.h>

//================================================================//
// MOAITextureLoader
//================================================================//
class MOAITextureLoader {
public:

	enum {
		TYPE_MOAI_IMAGE,
		TYPE_PVR,
		TYPE_UNKNOWN,
		TYPE_FAIL,
	};

	STLString		mFilename;
	void*			mFileData;
	size_t			mFileDataSize;
	USImage			mImage;
	u32				mTransform;
	u32				mType;
	
	//----------------------------------------------------------------//
	void Load ( u32 transform = 0 ) {
	
		if ( this->mType != TYPE_UNKNOWN ) {
			return;
		}
		
		this->mTransform |= transform;
		
		if ( !this->mImage.IsOK ()) {
	
			if ( this->mFileData ) {
				this->mImage.Load ( this->mFileData, ( u32 )this->mFileDataSize, this->mTransform );
				free ( this->mFileData );
				this->mFileData = 0;
			}
			else if ( mFilename.size ()) {
				this->mImage.Load ( this->mFilename, this->mTransform );
			}
		}
		
		if ( this->mImage.IsOK ()) {
			this->mType = TYPE_MOAI_IMAGE;
		}
		#ifdef MOAI_TEST_PVR
			else {
				// get file data, check if PVR
				USFileStream stream;
				stream.OpenRead ( this->mFilename );
	
				if ( this->mFileData ) {
					free ( this->mFileData );
					this->mFileData = 0;
				}
				
				this->mFileDataSize = stream.GetLength ();
				this->mFileData = malloc ( this->mFileDataSize );
				stream.ReadBytes ( this->mFileData, this->mFileDataSize );
	
				stream.Close ();
				
				if ( MOAIPvrHeader::GetHeader( this->mFileData, this->mFileDataSize )) {				
					this->mType = TYPE_PVR;
				}
			}
		#endif
		
		if ( this->mType == TYPE_UNKNOWN ) {
			this->mType = TYPE_FAIL;
			
			if ( this->mFileData ) {
				free ( this->mFileData );
				this->mFileData = 0;
			}
		}
	}
	
	//----------------------------------------------------------------//
	MOAITextureLoader () :
		mFileData ( 0 ),
		mFileDataSize ( 0 ),
		mType ( TYPE_UNKNOWN ) {
	}
	
	//----------------------------------------------------------------//
	~MOAITextureLoader () {
	
		if ( this->mFileData ) {
			free ( this->mFileData );
		}
	}
};

//================================================================//
// local
//================================================================//

//----------------------------------------------------------------//
/**	@name	bind
	@text	Try to force the texture to perform its load.
	
	@in		MOAITexture self
	@out	nil
*/
int MOAITexture::_bind ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAITexture, "U" )
	
	self->Bind ();
	
	return 0;
}

//----------------------------------------------------------------//
/**	@name	getSize
	@text	Returns the width and height of the texture's source image.
			Avoid using the texture width and height to compute UV
			coordinates from pixels, as this will prevent texture
			resolution swapping.
	
	@in		MOAITexture self
	@out	width
	@out	height
*/
int MOAITexture::_getSize ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAITexture, "U" )
	
	lua_pushnumber ( state, self->mWidth );
	lua_pushnumber ( state, self->mHeight );
	
	return 2;
}

//----------------------------------------------------------------//
/**	@name	load
	@text	Loads a texture from a data buffer or a file. Optionally pass
			in an image transform (affects .png images only).
	
	@in		MOAITexture self
	@in		variant texture		Either a MOAIDataBuffer containing a binary texture or a path to a texture file.
	@opt	number transform	Any bitwise combination of MOAIImage.QUANTIZE, MOAIImage.TRUECOLOR, MOAIImage.PREMULTIPLY_ALPHA
	@out	nil
*/
int MOAITexture::_load ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAITexture, "U" )

	MOAIDataBuffer* data = state.GetLuaObject < MOAIDataBuffer >( 2 );
	u32 transform = state.GetValue < u32 >( 3, DEFAULT_TRANSFORM );

	if ( data ) {

		self->Load ( *data, transform );
	}
	else if ( state.IsType( 2, LUA_TSTRING ) ) {

		cc8* filename = lua_tostring ( state, 2 );
		MOAI_CHECK_FILE ( filename );
		self->Load ( filename, transform );
	}

	return 0;
}

//----------------------------------------------------------------//
/**	@name	release
	@text	Releases any memory associated with the texture.
	
	@in		MOAITexture self
	@out	nil
*/
int MOAITexture::_release ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAITexture, "U" )
	
	self->MOAITexture::Release ();
	
	return 0;
}

//----------------------------------------------------------------//
/**	@name	setFilter
	@text	Set default filtering mode for texture.
	
	@in		MOAITexture self
	@in		number min			One of MOAITexture.GL_LINEAR, MOAITexture.GL_LINEAR_MIPMAP_LINEAR, MOAITexture.GL_LINEAR_MIPMAP_NEAREST,
								MOAITexture.GL_NEAREST, MOAITexture.GL_NEAREST_MIPMAP_LINEAR, MOAITexture.GL_NEAREST_MIPMAP_NEAREST
	@opt	number mag			Defaults to value passed to 'min'.
	@out	nil
*/
int MOAITexture::_setFilter ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAITexture, "UN" )

	int min = state.GetValue < int >( 2, GL_LINEAR );
	int mag = state.GetValue < int >( 3, min );

	self->SetFilter ( min, mag );

	return 0;
}

//----------------------------------------------------------------//
/**	@name	setFilter
	@text	Set wrapping mode for texture.
	
	@in		MOAITexture self
	@in		boolean wrap		Texture will wrap if true, clamp if not.
	@out	nil
*/
int MOAITexture::_setWrap ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAITexture, "UB" )
	
	bool wrap = state.GetValue < bool >( 2, false );
	
	self->mWrap = wrap ? GL_REPEAT : GL_CLAMP_TO_EDGE;

	return 0;
}


//================================================================//
// MOAITexture
//================================================================//

//----------------------------------------------------------------//
void MOAITexture::Affirm () {

	if ( this->mGLTexID ) return;
	
	if ( !this->mLoader ) {
		return;
	}
	
	//if ( !GLEW_OES_compressed_paletted_texture ) {
	//	transform.ConvertToTrueColor ();
	//}
	
	this->mLoader->Load ();
	
	switch ( this->mLoader->mType ) {
		
		case MOAITextureLoader::TYPE_MOAI_IMAGE: {
			this->CreateTextureFromImage ( this->mLoader->mImage );
			break;
		}
		
		case MOAITextureLoader::TYPE_PVR: {
			this->CreateTextureFromPVR ( this->mLoader->mFileData, this->mLoader->mFileDataSize );
			break;
		}
		default:
			delete this->mLoader;
			this->mLoader = 0;
	}

	if ( this->mGLTexID ) {

		// done with the loader
		delete this->mLoader;
		this->mLoader = 0;
	}
}

//----------------------------------------------------------------//
MOAITexture* MOAITexture::AffirmTexture ( USLuaState& state, int idx ) {

	MOAITexture* texture = state.GetLuaObject < MOAITexture >( idx );
	if ( !texture ) {

		u32 transform = state.GetValue < u32 >( idx + 1, DEFAULT_TRANSFORM );

		if ( state.IsType ( idx, LUA_TUSERDATA )) {
			
			texture = state.GetLuaObject < MOAITexture >( idx );
			MOAIImage* image = state.GetLuaObject < MOAIImage >( idx );
			
			if ( image ) {
				texture = new MOAITexture ();
				texture->Load ( *image );
			}
			else {
			
				MOAIDataBuffer* data = state.GetLuaObject < MOAIDataBuffer >( idx );
				
				if ( data ) {
					texture = new MOAITexture ();
					texture->Load ( *data, transform );
				}
			}
		}
		else if ( state.IsType ( idx, LUA_TSTRING )) {
			
			cc8* filename = lua_tostring ( state, idx );
			texture = new MOAITexture ();
			texture->Load ( filename, transform );
		}
	}
	return texture;
}

//----------------------------------------------------------------//
bool MOAITexture::Bind () {

	this->Affirm ();
	if ( !this->mGLTexID ) {
		return false;
	}

	// attempt to recover from lost context
	if ( !glIsTexture ( this->mGLTexID )) {
	
		this->mGLTexID = 0;
	
		// ugh... fix this monstrosity later!
		if ( this->mFilename.size ()) {
			this->Load ( this->mFilename );
			this->Affirm ();
		}
		if ( !this->mGLTexID ) return false;
	}

	glBindTexture ( GL_TEXTURE_2D, this->mGLTexID );
	glEnable ( GL_TEXTURE_2D );
	
	if ( MOAIGfxDevice::Get ().GetPipelineMode () == MOAIGfxDevice::GL_PIPELINE_FIXED ) {
		glTexEnvf ( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	}
	
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, this->mWrap );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, this->mWrap );
	
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, this->mMinFilter );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, this->mMagFilter );
	
	return true;
}

//----------------------------------------------------------------//
void MOAITexture::CreateTextureFromImage ( USImage& image ) {

	if ( !image.IsOK ()) return;

	// get the dimensions before trying to get the OpenGL texture ID
	this->mWidth = image.GetWidth ();
	this->mHeight = image.GetHeight ();

	glGenTextures ( 1, &this->mGLTexID );
	if ( !this->mGLTexID ) return;

	glBindTexture ( GL_TEXTURE_2D, this->mGLTexID );

	USPixel::Format pixelFormat = image.GetPixelFormat ();
	USColor::Format colorFormat = image.GetColorFormat ();

	// generate mipmaps if set up to use them
	if (	( this->mMinFilter == GL_LINEAR_MIPMAP_LINEAR ) ||
			( this->mMinFilter == GL_LINEAR_MIPMAP_NEAREST ) ||
			( this->mMinFilter == GL_NEAREST_MIPMAP_LINEAR ) ||
			( this->mMinFilter == GL_NEAREST_MIPMAP_NEAREST )) {
		
		glTexParameteri ( GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE );
	}

	if ( pixelFormat == USPixel::TRUECOLOR ) {

		// GL_ALPHA
		// GL_RGB
		// GL_RGBA
		// GL_LUMINANCE
		// GL_LUMINANCE_ALPHA

		// GL_UNSIGNED_BYTE
		// GL_UNSIGNED_SHORT_5_6_5
		// GL_UNSIGNED_SHORT_4_4_4_4
		// GL_UNSIGNED_SHORT_5_5_5_1

		switch ( colorFormat ) {
			
			case USColor::A_8:
				this->mGLInternalFormat = GL_ALPHA;
				this->mGLPixelType = GL_UNSIGNED_BYTE;
				break;
			
			case USColor::RGB_888:
				this->mGLInternalFormat = GL_RGB;
				this->mGLPixelType = GL_UNSIGNED_BYTE;
				break;
			
			case USColor::RGB_565:
				this->mGLInternalFormat = GL_RGB;
				this->mGLPixelType = GL_UNSIGNED_SHORT_5_6_5;
				break;
			
			case USColor::RGBA_4444:
				this->mGLInternalFormat = GL_RGBA;
				this->mGLPixelType = GL_UNSIGNED_SHORT_4_4_4_4;
				break;
			
			case USColor::RGBA_8888:
				this->mGLInternalFormat = GL_RGBA;
				this->mGLPixelType = GL_UNSIGNED_BYTE;
				break;
				
			default: return;
		}

		glTexImage2D (
			GL_TEXTURE_2D,
			0,  
			this->mGLInternalFormat,
			this->mWidth,  
			this->mHeight,  
			0,  
			this->mGLInternalFormat,
			this->mGLPixelType,  
			image.GetBitmap ()
		);
	}
	else {
	
		// GL_PALETTE4_RGB8_OES
		// GL_PALETTE4_R5_G6_B5_OES
		// GL_PALETTE4_RGBA4_OES
		// GL_PALETTE4_RGB5_A1_OES
		// GL_PALETTE4_RGBA8_OES

		// GL_PALETTE8_RGB8_OES
		// GL_PALETTE8_R5_G6_B5_OES
		// GL_PALETTE8_RGBA4_OES
		// GL_PALETTE8_RGB5_A1_OES
		// GL_PALETTE8_RGBA8_OES

// TODO: check opengl version
//		this->mGLPixelType = 0;
//	
//		if ( pixelFormat == USPixel::INDEX_4 ) {
//		
//			switch ( colorFormat ) {
//			
//				case USColor::RGB_888:
//					this->mGLInternalFormat = GL_PALETTE4_RGB8_OES;
//					break;
//				
//				case USColor::RGBA_8888:
//					this->mGLInternalFormat = GL_PALETTE4_RGBA8_OES;
//					break;
//					
//				default: return;
//			}
//		}
//		else {
//			switch ( colorFormat ) {
//			
//				case USColor::RGB_888:
//					this->mGLInternalFormat = GL_PALETTE8_RGB8_OES;
//					break;
//				
//				case USColor::RGBA_8888:
//					this->mGLInternalFormat = GL_PALETTE8_RGBA8_OES;
//					break;
//					
//				default: return;
//			}
//		}
//		
//		glCompressedTexImage2D (
//			GL_TEXTURE_2D,
//			0,  
//			this->mGLInternalFormat,
//			this->mDevWidth,
//			this->mDevHeight,
//			0,  
//			image.GetDataSize (),
//			image.GetData ()
//		);
	}
}

//----------------------------------------------------------------//
void MOAITexture::CreateTextureFromPVR ( void* data, size_t size ) {
	UNUSED ( data );
	UNUSED ( size );

	#ifdef MOAI_TEST_PVR

		MOAIPvrHeader* header = MOAIPvrHeader::GetHeader ( data, size );
		if ( !header ) return;
		
		bool compressed = false;
		bool hasAlpha = header->mAlphaBitMask ? true : false;
		
		switch ( header->mPFFlags & MOAIPvrHeader::PF_MASK ) {
			
			case MOAIPvrHeader::OGL_RGBA_4444:
				compressed = false;
				this->mGLInternalFormat = GL_RGBA;
				this->mGLPixelType = GL_UNSIGNED_SHORT_4_4_4_4;
				break;
		
			case MOAIPvrHeader::OGL_RGBA_5551:
				compressed = false;
				this->mGLInternalFormat = GL_RGBA;
				this->mGLPixelType = GL_UNSIGNED_SHORT_5_5_5_1;
				break;
			
			case MOAIPvrHeader::OGL_RGBA_8888:
				compressed = false;
				this->mGLInternalFormat = GL_RGBA;
				this->mGLPixelType = GL_UNSIGNED_BYTE;
				break;
			
			case MOAIPvrHeader::OGL_RGB_565:
				compressed = false;
				this->mGLInternalFormat = GL_RGB;
				this->mGLPixelType = GL_UNSIGNED_SHORT_5_6_5;
				break;
			
			// NO IMAGE FOR THIS
//			case MOAIPvrHeader::OGL_RGB_555:
//				break;
			
			case MOAIPvrHeader::OGL_RGB_888:
				compressed = false;
				this->mGLInternalFormat = GL_RGB;
				this->mGLPixelType = GL_UNSIGNED_BYTE;
				break;
			
			case MOAIPvrHeader::OGL_I_8:
				compressed = false;
				this->mGLInternalFormat = GL_LUMINANCE;
				this->mGLPixelType = GL_UNSIGNED_BYTE;
				break;
			
			case MOAIPvrHeader::OGL_AI_88:
				compressed = false;
				this->mGLInternalFormat = GL_LUMINANCE_ALPHA;
				this->mGLPixelType = GL_UNSIGNED_BYTE;
				break;
			
			case MOAIPvrHeader::OGL_PVRTC2:
				compressed = true;
				this->mGLInternalFormat = hasAlpha ? GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG : GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
				break;
			
			case MOAIPvrHeader::OGL_PVRTC4:
				compressed = true;
				this->mGLInternalFormat = hasAlpha ? GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG : GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
				break;
			
			case MOAIPvrHeader::OGL_BGRA_8888:
				compressed = false;
				this->mGLInternalFormat = GL_BGRA;
				this->mGLPixelType = GL_UNSIGNED_BYTE;
				break;
			
			case MOAIPvrHeader::OGL_A_8:
				compressed = false;
				this->mGLInternalFormat = GL_ALPHA;
				this->mGLPixelType = GL_UNSIGNED_BYTE;
				break;
		}
		
		
		glGenTextures ( 1, &this->mGLTexID );
		if ( !this->mGLTexID ) return;

		glBindTexture ( GL_TEXTURE_2D, this->mGLTexID );
		
		int width = header->mWidth;
		int height = header->mHeight;		
		char* imageData = (char*)(header->GetFileData ( data, size));
		if ( header->mMipMapCount == 0 ) {
			GLsizei currentSize = (GLsizei) USFloat::Max ( (float)(32), (float)(width * height * header->mBitCount / 8) );
			
			if ( compressed ) {
				glCompressedTexImage2D ( GL_TEXTURE_2D, 0, this->mGLInternalFormat, width, height, 0, currentSize, imageData );
			}
			else {
				glTexImage2D( GL_TEXTURE_2D, 0, this->mGLInternalFormat, width, height, 0, this->mGLInternalFormat, this->mGLPixelType, imageData);
			}
		}
		else {
			for ( int level = 0; width > 0 && height > 0; ++level ) {
				GLsizei currentSize = (GLsizei) USFloat::Max ( (float)(32), (float)(width * height * header->mBitCount / 8) );
			
				if ( compressed ) {
					glCompressedTexImage2D ( GL_TEXTURE_2D, level, this->mGLInternalFormat, width, height, 0, currentSize, imageData );
				}
				else {
					glTexImage2D( GL_TEXTURE_2D, level, this->mGLInternalFormat, width, height, 0, this->mGLInternalFormat, this->mGLPixelType, imageData);
				}
			
				imageData += currentSize;
				width >>= 1;
				height >>= 1;
			}	
		}			

	#endif
}

//----------------------------------------------------------------//
u32 MOAITexture::GetHeight () {
	return this->mHeight;
}

//----------------------------------------------------------------//
u32 MOAITexture::GetWidth () {
	return this->mWidth;
}

//----------------------------------------------------------------//
bool MOAITexture::IsOK () {
 
	return (( this->mLoader ) || ( this->mGLTexID != 0 ));
}

//----------------------------------------------------------------//
void MOAITexture::Load ( MOAIImage& image ) {

	this->Release ();
	this->mLoader = new MOAITextureLoader ();
	
	this->mLoader->mTransform = 0;
	this->mLoader->mImage.Copy ( image );
	
	this->Bind ();
}

//----------------------------------------------------------------//
void MOAITexture::Load ( cc8* filename, u32 transform ) {

	this->Release ();
	if ( !USFileSys::CheckFileExists ( filename )) return;

	this->mTexturePath = USFileSys::Expand ( filename );
	
	this->mLoader = new MOAITextureLoader ();
	this->mLoader->mTransform = transform;
	this->mLoader->mFilename = this->mFilename;
	
	this->Bind ();
}


//----------------------------------------------------------------//
void MOAITexture::Load ( MOAIDataBuffer& data, u32 transform ) {

	void* bytes;
	u32 size;
	data.Lock ( &bytes, &size );

	this->Load ( bytes, size, transform );
	
	data.Unlock ();
}

//----------------------------------------------------------------//
void MOAITexture::Load ( const void* data, u32 size, u32 transform ) {

	this->Release ();
	this->mLoader = new MOAITextureLoader ();
	
	this->mLoader->mTransform = transform;
	this->mLoader->mFileDataSize = size;
	this->mLoader->mFileData = malloc ( size );
	memcpy ( this->mLoader->mFileData, data, size );
	
	this->Bind ();
}

//----------------------------------------------------------------//
MOAITexture::MOAITexture () :
	mGLTexID ( 0 ),
	mWidth ( 0 ),
	mHeight ( 0 ),
	mMinFilter ( GL_LINEAR ),
	mMagFilter ( GL_NEAREST ),
	mWrap ( GL_CLAMP_TO_EDGE ),
	mLoader ( 0 ) {
	
	RTTI_SINGLE ( USLuaObject )
}

//----------------------------------------------------------------//
MOAITexture::~MOAITexture () {

	this->Release ();
}

//----------------------------------------------------------------//
void MOAITexture::RegisterLuaClass ( USLuaState& state ) {
	
	state.SetField ( -1, "GL_LINEAR", ( u32 )GL_LINEAR );
	state.SetField ( -1, "GL_LINEAR_MIPMAP_LINEAR", ( u32 )GL_LINEAR_MIPMAP_LINEAR );
	state.SetField ( -1, "GL_LINEAR_MIPMAP_NEAREST", ( u32 )GL_LINEAR_MIPMAP_NEAREST );
	
	state.SetField ( -1, "GL_NEAREST", ( u32 )GL_NEAREST );
	state.SetField ( -1, "GL_NEAREST_MIPMAP_LINEAR", ( u32 )GL_NEAREST_MIPMAP_LINEAR );
	state.SetField ( -1, "GL_NEAREST_MIPMAP_NEAREST", ( u32 )GL_NEAREST_MIPMAP_NEAREST );
}

//----------------------------------------------------------------//
void MOAITexture::RegisterLuaFuncs ( USLuaState& state ) {

	luaL_Reg regTable [] = {
		{ "bind",				_bind },
		{ "getSize",			_getSize },
		{ "load",				_load },
		{ "release",			_release },
		{ "setFilter",			_setFilter },
		{ "setWrap",			_setWrap },
		{ NULL, NULL }
	};

	luaL_register ( state, 0, regTable );
}

//----------------------------------------------------------------//
void MOAITexture::Release () {

	if ( this->mGLTexID ) {
		glDeleteTextures ( 1, &this->mGLTexID );
		this->mGLTexID = 0;
	}
	
	this->mWidth = 0;
	this->mHeight = 0;
	
	if ( this->mLoader ) {
		delete this->mLoader;
		this->mLoader = 0;
	}
}

//----------------------------------------------------------------//
void MOAITexture::SerializeIn ( USLuaState& state, USLuaSerializer& serializer ) {
	UNUSED ( serializer );

	STLString path = state.GetField ( -1, "mPath", "" );
	
	if ( path.size ()) {
		USFilename filename;
		filename.Bless ( path.str ());
		this->Load ( filename.mBuffer, DEFAULT_TRANSFORM ); // TODO: serialization
	}
}

//----------------------------------------------------------------//
void MOAITexture::SerializeOut ( USLuaState& state, USLuaSerializer& serializer ) {
	UNUSED ( serializer );

	STLString path = USFileSys::GetRelativePath ( this->mTexturePath );
	state.SetField ( -1, "mPath", path.str ());
}

//----------------------------------------------------------------//
void MOAITexture::SetFilter ( int filter ) {

	this->SetFilter ( filter, filter );
}

//----------------------------------------------------------------//
void MOAITexture::SetFilter ( int min, int mag ) {

	this->mMinFilter = min;
	this->mMagFilter = mag;
}

//----------------------------------------------------------------//
void MOAITexture::SetWrap ( int wrap ) {

	this->mWrap = wrap;
}

//----------------------------------------------------------------//
STLString MOAITexture::ToString () {

	STLString repr;

	PrettyPrint ( repr, "mWidth", GetWidth ());
	PrettyPrint ( repr, "mHeight", GetHeight ());

	return repr;
}
