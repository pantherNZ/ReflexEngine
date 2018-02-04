// Self Include:
#include "Game.h"

// Local Includes:
#include "System.h"

// Library Includes:

// Statics:

// Implementation:
CGame::CGame()
	: m_pDevice(nullptr)
	, m_pDirectXClass(nullptr)
	, m_pSpriteShader(nullptr)
	, m_pCameraClass(nullptr)
	, m_pSpriteInstance(nullptr)
	, m_hWnd(NULL)
	, m_fWidth(0.0f)
	, m_fHeight(0.0f)		
{	

}

CGame::~CGame()
{
	// Delete classes used
	DeleteOBJ(m_pDirectXClass);
	DeleteOBJ(m_pCameraClass);
	DeleteOBJ(m_pSpriteShader);

	// Release sprite buffers
	CSprite::ReleaseBuffers();

	DeleteOBJ(m_pSpriteInstance);
}

bool CGame::Initialise(HINSTANCE _hInstance, HWND _hWnd, int _iWidth, int _iHeight)
{
	m_fWidth = ( float )_iWidth;
	m_fHeight = ( float )_iHeight;
	m_hWnd = _hWnd;

	// Create dx10 class
	m_pDirectXClass = new CDirectX;

	// Initalise Dx10 class
	if(!m_pDirectXClass->Initialise(_hWnd, _iWidth, _iHeight))
	{
		MessageBox(NULL, L"Initialising DirectX10 Failed", L"Error", MB_OK);	
		return(false);
	}

	// Store created device
	m_pDevice = m_pDirectXClass->GetDevice(); 

	// Create camera class
	m_pCameraClass = new CCamera;

	// Initialise camera
	m_pCameraClass->Initialise(_iWidth, _iHeight);

	// Initialise sprite buffers
	CSprite::CreateBuffers(m_pDevice);
	CSprite::ApplyRenderOptions();

	// Create sprite shader class
	m_pSpriteShader = new CSpriteShader;

	// Initialise sprite shader class
	if(!m_pSpriteShader->Initialise(m_pDevice, _hWnd))
	{
		MessageBox(NULL, L"Initialising Sprite Shader Failed", L"Error", MB_OK);	
		return(false);
	}
	
	// Create Game Sprites
	m_pSpriteInstance = new CSprite;
	m_pSpriteInstance->Initialise(L"Data\\Sprites\\Square.bmp", D3DXVECTOR2(0.0f, 0.0f), 0.0f, D3DXVECTOR2(0.2f, 0.2f));

	ParseFile( "Data\\VirtualStats.cpp" );
	GenerateGraph( );

	return(true);
}

void CGame::ParseFile( const std::string& _rstrFileName )
{
	std::ifstream input( _rstrFileName );

	std::string strVirtual = "VIRTUAL_STAT(";
	std::string strLine;
    int iCurrentNodeIndex = 0;
           
    bool bHaveCurrentNode = false;

	while( !input.eof() )
	{
		// Process each line
		std::getline(input, strLine);

		// Trim
		strLine = Trim( strLine );

        // Check for a virtual stat line
		if( strLine.substr( 0, strVirtual.size( ) ) == strVirtual )
        {
            // Narrow down to find the stat name
			std::string strStatName = strLine.substr( strVirtual.size( ), strLine.size( ) - strVirtual.size( ) );
			int uiCounter = IndexOf( strLine, ',' );
            strStatName = strStatName.substr( 0, ( uiCounter == -1 ? strStatName.size( ) : uiCounter ) );
            strStatName = Trim( strStatName );

            // Add the virtual stat to the list of initial nodes (if it doesn't already exist)
			const auto found = std::find_if( m_vecNodes.begin( ), m_vecNodes.end( ), [&strStatName]( const Node* _p ) -> bool
			{ 
				return _p->strName == strStatName; 
			} );
			
			if( found == m_vecNodes.end( ) )
			{
				iCurrentNodeIndex = m_vecNodes.size( );
				auto* pNewNode = new Node( strStatName );
				m_vecNodes.push_back( pNewNode );             
				bHaveCurrentNode = true;
			}
        }
        else if( bHaveCurrentNode )
        {
            // Split the line incase there is multiple stats on a line
            std::vector< std::string > strStatsArray = Split( strLine, ',' );
            bool bVirtualStatComplete = false;

            // For eachstat
			for( auto strStat : strStatsArray )
            {
                // Trim space
				auto strStatTrimmed = Trim( strStat );

                if( strStatTrimmed.size( ) == 0 )
                    continue;

                if( strStatTrimmed.back( ) == ')' )
                {
                    bVirtualStatComplete = true;
					strStatTrimmed.pop_back( );
					strStatTrimmed = Trim( strStatTrimmed );
                }
				
                // Find a matching stat in our list
                bool bFound = false;
				
                for( auto* statNode : m_vecNodes )
                {
                    if( statNode->strName == strStatTrimmed )
                    {
						m_vecNodes[iCurrentNodeIndex]->vecConnections.push_back( statNode );
                        bFound = true;
                        break;
                    }
                }

                // If we didn't find a matching stat, create a new one
                if( !bFound )
                {
                    Node* pNewNode = new Node( strStatTrimmed );
					m_vecNodes.push_back( pNewNode );
                    m_vecNodes[iCurrentNodeIndex]->vecConnections.push_back( pNewNode );
                }
            }

            if( bVirtualStatComplete )
                bHaveCurrentNode = false;
        }
    }

    input.close();
}

void CGame::GenerateGraph( )
{
	const int iRadius = 2000;
	const float fAngle = ( 2.0f * ( float )M_PI ) / m_vecNodes.size( );
	const D3DXVECTOR4 vec4Bounds( -m_fWidth / 2.0f, -m_fHeight / 2.0f, m_fWidth / 2.0f, m_fHeight / 2.0f );
	float fCurrentAngle = 0.0f;

	for( auto* node : m_vecNodes )
	{
		node->vec2Position = D3DXVECTOR2( Rand( vec4Bounds.x, vec4Bounds.z ), Rand( vec4Bounds.y, vec4Bounds.w ) );
		//node->vec2Position = D3DXVECTOR2( cosf( fCurrentAngle ) * iRadius, sinf( fCurrentAngle ) * iRadius );
		//fCurrentAngle += fAngle;
	}
}

void CGame::Process(float _fDelta)
{
	m_pCameraClass->Process(_fDelta);
}

void CGame::Render()
{
	// Begin scene
	m_pDirectXClass->BeginScene(D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f));

	// Render no$des
	for( auto* node : m_vecNodes )
	{ 
		m_pSpriteInstance->SetPosition( node->vec2Position );
		RenderSprite( m_pSpriteInstance );
	}

	// End scene
	m_pDirectXClass->EndScene();
}

void CGame::RenderSprite(void* _pSprite)
{
	CSprite* pSprite = static_cast<CSprite*>(_pSprite);

	m_pSpriteShader->Render(pSprite->GetWorldMatrix(), m_pCameraClass->GetViewMatrix(), m_pCameraClass->GetProjectionMatrix(),
							pSprite->GetTextureMatrix(), pSprite->GetTexture(), pSprite->GetFaceCount());
}