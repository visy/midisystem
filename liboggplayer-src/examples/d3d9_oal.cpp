//=========================================================================
// Author      : Kambiz Veshgini
// Description : Simple theora plyer using direct3d 9 and openal
//=========================================================================

#include <d3d9.h>
#include <D3dx9math.h>
#pragma warning( disable : 4996 ) // Disable deprecated warning 
#include <strsafe.h>
#pragma warning( default : 4996 )
#include <oggplayer.h>
#include <alut.h>

// Use pragma for linking (MSVC only)
#pragma comment(lib,"d3d9.lib")
#pragma comment(lib,"d3dx9.lib")
#ifdef _DEBUG
	#pragma comment(lib,"liboggplayer-d.lib")
#else
	#pragma comment(lib,"liboggplayer.lib")
#endif
#pragma comment(lib,"alut.lib")
#pragma comment(lib,"OpenAL32.lib")

LPDIRECT3D9             g_pD3D = NULL;       
LPDIRECT3DDEVICE9       g_pd3dDevice = NULL; 
LPDIRECT3DVERTEXBUFFER9 g_pVB = NULL;        
LPDIRECT3DTEXTURE9      g_pTexture = NULL;   

struct CUSTOMVERTEX
{
	FLOAT x, y, z, w;
	D3DCOLOR color;
	FLOAT tu, tv;
};


#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1)


OggPlayer g_ogg; 

HRESULT InitD3D( HWND hWnd )
{
	// Create the D3D object.
	if( NULL == ( g_pD3D = Direct3DCreate9( D3D_SDK_VERSION ) ) )
		return E_FAIL;

	// Set up the structure used to create the D3DDevice
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory( &d3dpp, sizeof( d3dpp ) );
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

	// Create the D3DDevice
	if( FAILED( g_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp, &g_pd3dDevice ) ) )
	{
		return E_FAIL;
	}

	g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

	// Create a dynamic texture
	if( FAILED( D3DXCreateTexture(g_pd3dDevice,g_ogg.width(),g_ogg.height(),1,D3DUSAGE_DYNAMIC,D3DFMT_X8B8G8R8,D3DPOOL_DEFAULT,&g_pTexture)))
	{
		return E_FAIL;
	}

	float x1 = 0;
	float y1 = 0;
	float x2 = g_ogg.width();
	float y2 = g_ogg.height();
	// Initialize three vertices for rendering a quad
	CUSTOMVERTEX vertices[] =
	{   // x, y, z, color, tu, tv
		{  x1,  y1, 0.0f, 1.0f, 0xffffffff, 0.0f, 0.0f }, 
		{  x2,  y1, 0.0f, 1.0f, 0xffffffff, 1.0f, 0.0f },
		{  x2,  y2, 0.0f, 1.0f, 0xffffffff, 1.0f, 1.0f },
		{  x1,  y2, 0.0f, 1.0f, 0xffffffff, 0.0f, 1.0f },
	};

	if( FAILED( g_pd3dDevice->CreateVertexBuffer( 4 * sizeof( CUSTOMVERTEX ),
		0, D3DFVF_CUSTOMVERTEX,
		D3DPOOL_DEFAULT, &g_pVB, NULL ) ) )
	{
		return E_FAIL;
	}

	VOID* pVertices;
	if( FAILED( g_pVB->Lock( 0, sizeof( vertices ), ( void** )&pVertices, 0 ) ) )
		return E_FAIL;
	memcpy( pVertices, vertices, sizeof( vertices ) );
	g_pVB->Unlock();

	return S_OK;
}

bool InitAL()
{
	alutInit(0, NULL);

	int error = alGetError();
	return error == AL_NO_ERROR ;
}

VOID Cleanup()
{
	if( g_pTexture != NULL )
		g_pTexture->Release();

	if( g_pVB != NULL )
		g_pVB->Release();

	if( g_pd3dDevice != NULL )
		g_pd3dDevice->Release();

	if( g_pD3D != NULL )
		g_pD3D->Release();

}

VOID Render()
{
	D3DLOCKED_RECT lr;
	if(SUCCEEDED(g_pTexture->LockRect(0,&lr,NULL,0)))
	{
		g_ogg.video_read((char*)lr.pBits,lr.Pitch);
		g_pTexture->UnlockRect(0);
	}

	if( SUCCEEDED( g_pd3dDevice->BeginScene() ) )
	{
		g_pd3dDevice->SetTexture(0,g_pTexture);

		g_pd3dDevice->SetStreamSource( 0, g_pVB, 0, sizeof( CUSTOMVERTEX ) );
		g_pd3dDevice->SetFVF( D3DFVF_CUSTOMVERTEX );
		g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLEFAN, 0, 2 );

		g_pd3dDevice->EndScene();
	}

	g_pd3dDevice->Present( NULL, NULL, NULL, NULL );
}

DWORD WINAPI MixAudio(LPVOID lpParameter )
{
	const int BUFFER_SIZE = 4096;
	const int NUM_BUFFERS = 8;
	char* pcm = new char[BUFFER_SIZE];
	ALuint buffers[NUM_BUFFERS];
	ALuint source;
	int size;

	alGenBuffers(NUM_BUFFERS, buffers);
	alGenSources(1, &source);

	for(int i=0;i<NUM_BUFFERS;i++){
		size = g_ogg.audio_read(pcm,BUFFER_SIZE);
		alBufferData(buffers[i], AL_FORMAT_STEREO16, pcm, size, 44100);
	}
	alSourceQueueBuffers(source, NUM_BUFFERS, buffers);
	alSourcePlay(source);
	while(g_ogg.playing())
	{
		int processed;
		alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);
		
		while((processed--)>0)
		{
			ALuint buffer;
			alSourceUnqueueBuffers(source, 1, &buffer);
			size = g_ogg.audio_read(pcm,BUFFER_SIZE);
			if(size>0)
				alBufferData(buffer, AL_FORMAT_STEREO16, pcm, size, 44100);
			alSourceQueueBuffers(source, 1, &buffer);
			ALenum state;
			alGetSourcei(source, AL_SOURCE_STATE, &state);
			if(state!=AL_PLAYING)
				alSourcePlay(source);
		}
		Sleep(5);
	}
	alSourceStop(source);
	alDeleteSources(1, &source);
	alDeleteBuffers(NUM_BUFFERS, buffers);
	delete [] pcm;
	return 0;
}

LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
	case WM_DESTROY:
		Cleanup();
		PostQuitMessage( 0 );
		return 0;
	}

	return DefWindowProc( hWnd, msg, wParam, lParam );
}


INT WINAPI wWinMain( HINSTANCE hInst, HINSTANCE, LPWSTR, INT )
{
	g_ogg.open("../sample video/trailer_400p.ogg",AF_S16,2,44100,VF_BGRA);
	if(g_ogg.fail()) {
		return -1;
	}

	// Register the window class
	WNDCLASSEX wc =
	{
		sizeof( WNDCLASSEX ), CS_CLASSDC, MsgProc, 0L, 0L,
		GetModuleHandle( NULL ), NULL, NULL, NULL, NULL,
		L"liboggstream", NULL
	};

	RegisterClassEx( &wc );


	int width=g_ogg.width();
	int height=g_ogg.height();
	// Create the application's window
	RECT rect;
	rect.left = 100;
	rect.top = 100;
	rect.right = width+rect.left;
	rect.bottom = height+rect.top;
	AdjustWindowRect(&rect,WS_OVERLAPPEDWINDOW,FALSE);
	HWND hWnd = CreateWindow( L"liboggstream", L"D3D9 Theora Player",
		WS_OVERLAPPEDWINDOW, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top,
		NULL, NULL, wc.hInstance, NULL );

	// Initialize Direct3D and OpenAL
	if( SUCCEEDED( InitD3D( hWnd ) ) && InitAL())
	{
		// Show the window
		ShowWindow( hWnd, SW_SHOWDEFAULT );
		UpdateWindow( hWnd );
		// Start the playback
		g_ogg.play();
		// Run MixAudio in another thread
		HANDLE h_mixaudio= CreateThread(NULL,0,MixAudio,NULL,0,NULL);
		// Enter the message loop
		MSG msg;
		ZeroMemory( &msg, sizeof( msg ) );
		while( msg.message != WM_QUIT && g_ogg.playing() )
		{
			if( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) )
			{
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}else {
				Render();
				Sleep(0);
			}
		}
		g_ogg.close();
		WaitForSingleObject(h_mixaudio,INFINITE);
		alutExit();
	}

	UnregisterClass( L"liboggstream", wc.hInstance );
	return 0;
}
