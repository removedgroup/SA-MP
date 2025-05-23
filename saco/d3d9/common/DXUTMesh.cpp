//-----------------------------------------------------------------------------
// File: DXUTMesh.cpp
//
// Desc: Support code for loading DirectX .X files.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "dxstdafx.h"
#include <dxfile.h>
#include <rmxfguid.h>
#include <rmxftmpl.h>
#include "DXUTMesh.h"
#undef min // use __min instead
#undef max // use __max instead




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
CDXUTMesh::CDXUTMesh( LPCTSTR strName )
{
    StringCchCopy( m_strName, 512, strName );
    m_pSysMemMesh        = NULL;
    m_pLocalMesh         = NULL;
    m_dwNumMaterials     = 0L;
    m_pMaterials         = NULL;
    m_pTextures          = NULL;
    m_bUseMaterials      = TRUE;
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
CDXUTMesh::~CDXUTMesh()
{
    Destroy();
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
HRESULT CDXUTMesh::Create( LPDIRECT3DDEVICE9 pd3dDevice, LPCTSTR strFilename )
{
    TCHAR        strPath[MAX_PATH];
    LPD3DXBUFFER pAdjacencyBuffer = NULL;
    LPD3DXBUFFER pMtrlBuffer = NULL;
    HRESULT      hr;

    // Find the path for the file, and convert it to ANSI (for the D3DX API)
    DXUTFindDXSDKMediaFileCch( strPath, sizeof(strPath) / sizeof(TCHAR), strFilename );

    // Load the mesh
    if( FAILED( hr = D3DXLoadMeshFromX( strPath, D3DXMESH_SYSTEMMEM, pd3dDevice, 
                                        &pAdjacencyBuffer, &pMtrlBuffer, NULL,
                                        &m_dwNumMaterials, &m_pSysMemMesh ) ) )
    {
        return hr;
    }

    // Optimize the mesh for performance
    if( FAILED( hr = m_pSysMemMesh->OptimizeInplace(
                        D3DXMESHOPT_COMPACT | D3DXMESHOPT_ATTRSORT | D3DXMESHOPT_VERTEXCACHE,
                        (DWORD*)pAdjacencyBuffer->GetBufferPointer(), NULL, NULL, NULL ) ) )
    {
        SAFE_RELEASE( pAdjacencyBuffer );
        SAFE_RELEASE( pMtrlBuffer );
        return hr;
    }

    // Set strPath to the path of the mesh file
    TCHAR *pLastBSlash = strchr( strPath, L'\\' );
    if( pLastBSlash )
        *(pLastBSlash + 1) = L'\0';
    else
        *strPath = L'\0';

    hr = CreateMaterials( strPath, pd3dDevice, pAdjacencyBuffer, pMtrlBuffer );

    SAFE_RELEASE( pAdjacencyBuffer );
    SAFE_RELEASE( pMtrlBuffer );

    return hr;
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
HRESULT CDXUTMesh::Create( LPDIRECT3DDEVICE9 pd3dDevice,
                          LPD3DXFILEDATA pFileData )
{
    LPD3DXBUFFER pMtrlBuffer = NULL;
    LPD3DXBUFFER pAdjacencyBuffer = NULL;
    HRESULT      hr;

    // Load the mesh from the DXFILEDATA object
    if( FAILED( hr = D3DXLoadMeshFromXof( pFileData, D3DXMESH_SYSTEMMEM, pd3dDevice,
                                          &pAdjacencyBuffer, &pMtrlBuffer, NULL,
                                          &m_dwNumMaterials, &m_pSysMemMesh ) ) )
    {
        return hr;
    }

    // Optimize the mesh for performance
    if( FAILED( hr = m_pSysMemMesh->OptimizeInplace(
                        D3DXMESHOPT_COMPACT | D3DXMESHOPT_ATTRSORT | D3DXMESHOPT_VERTEXCACHE,
                        (DWORD*)pAdjacencyBuffer->GetBufferPointer(), NULL, NULL, NULL ) ) )
    {
        SAFE_RELEASE( pAdjacencyBuffer );
        SAFE_RELEASE( pMtrlBuffer );
        return hr;
    }

    hr = CreateMaterials( "", pd3dDevice, pAdjacencyBuffer, pMtrlBuffer );

    SAFE_RELEASE( pAdjacencyBuffer );
    SAFE_RELEASE( pMtrlBuffer );

    return hr;
}

// MATCH
HRESULT CDXUTMesh::CreateMaterials( LPCTSTR strPath, IDirect3DDevice9 *pd3dDevice, ID3DXBuffer *pAdjacencyBuffer, ID3DXBuffer *pMtrlBuffer )
{
    // Get material info for the mesh
    // Get the array of materials out of the buffer
    if( pMtrlBuffer && m_dwNumMaterials > 0 )
    {
        // Allocate memory for the materials and textures
        D3DXMATERIAL* d3dxMtrls = (D3DXMATERIAL*)pMtrlBuffer->GetBufferPointer();
        m_pMaterials = new D3DMATERIAL9[m_dwNumMaterials];
        if( m_pMaterials == NULL )
            return E_OUTOFMEMORY;
        m_pTextures = new LPDIRECT3DBASETEXTURE9[m_dwNumMaterials];
        if( m_pTextures == NULL )
            return E_OUTOFMEMORY;

        // Copy each material and create its texture
        for( DWORD i=0; i<m_dwNumMaterials; i++ )
        {
            // Copy the material
            m_pMaterials[i]         = d3dxMtrls[i].MatD3D;
            m_pTextures[i]          = NULL;

            // Create a texture
            if( d3dxMtrls[i].pTextureFilename )
            {
                TCHAR strTexture[MAX_PATH];
                TCHAR strTextureTemp[MAX_PATH];
                D3DXIMAGE_INFO ImgInfo;

                // First attempt to look for texture in the same folder as the input folder.
                strcpy(strTextureTemp,d3dxMtrls[i].pTextureFilename);
                strTextureTemp[MAX_PATH-1] = 0;

                StringCchCopy( strTexture, MAX_PATH, strPath );
                StringCchCat( strTexture, MAX_PATH, strTextureTemp );

                // Inspect the texture file to determine the texture type.
                if( FAILED( D3DXGetImageInfoFromFile( strTexture, &ImgInfo ) ) )
                {
                    // Search the media folder
                    if( FAILED( DXUTFindDXSDKMediaFileCch( strTexture, MAX_PATH, strTextureTemp ) ) )
                        continue;  // Can't find. Skip.

                    D3DXGetImageInfoFromFile( strTexture, &ImgInfo );
                }

                // Call the appropriate loader according to the texture type.
                switch( ImgInfo.ResourceType )
                {
                    case D3DRTYPE_TEXTURE:
                    {
                        IDirect3DTexture9 *pTex;
                        if( SUCCEEDED( D3DXCreateTextureFromFile( pd3dDevice, strTexture, &pTex ) ) )
                        {
                            // Obtain the base texture interface
                            pTex->QueryInterface( IID_IDirect3DBaseTexture9, (LPVOID*)&m_pTextures[i] );
                            // Release the specialized instance
                            pTex->Release();
                        }
                        break;
                    }
                    case D3DRTYPE_CUBETEXTURE:
                    {
                        IDirect3DCubeTexture9 *pTex;
                        if( SUCCEEDED( D3DXCreateCubeTextureFromFile( pd3dDevice, strTexture, &pTex ) ) )
                        {
                            // Obtain the base texture interface
                            pTex->QueryInterface( IID_IDirect3DBaseTexture9, (LPVOID*)&m_pTextures[i] );
                            // Release the specialized instance
                            pTex->Release();
                        }
                        break;
                    }
                    case D3DRTYPE_VOLUMETEXTURE:
                    {
                        IDirect3DVolumeTexture9 *pTex;
                        if( SUCCEEDED( D3DXCreateVolumeTextureFromFile( pd3dDevice, strTexture, &pTex ) ) )
                        {
                            // Obtain the base texture interface
                            pTex->QueryInterface( IID_IDirect3DBaseTexture9, (LPVOID*)&m_pTextures[i] );
                            // Release the specialized instance
                            pTex->Release();
                        }
                        break;
                    }
                }
            }
        }
    }
    return S_OK;
}

//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
HRESULT CDXUTMesh::SetFVF( LPDIRECT3DDEVICE9 pd3dDevice, DWORD dwFVF )
{
    LPD3DXMESH pTempSysMemMesh = NULL;
    LPD3DXMESH pTempLocalMesh  = NULL;

    if( m_pSysMemMesh )
    {
        if( FAILED( m_pSysMemMesh->CloneMeshFVF( m_pSysMemMesh->GetOptions(), dwFVF,
                                                 pd3dDevice, &pTempSysMemMesh ) ) )
            return E_FAIL;
    }
    if( m_pLocalMesh )
    {
        if( FAILED( m_pLocalMesh->CloneMeshFVF( m_pLocalMesh->GetOptions(), dwFVF, pd3dDevice,
                                                &pTempLocalMesh ) ) )
        {
            SAFE_RELEASE( pTempSysMemMesh );
            return E_FAIL;
        }
    }

    DWORD dwOldFVF = 0;

    if( m_pSysMemMesh )
        dwOldFVF = m_pSysMemMesh->GetFVF();

    SAFE_RELEASE( m_pSysMemMesh );
    SAFE_RELEASE( m_pLocalMesh );

    if( pTempSysMemMesh ) m_pSysMemMesh = pTempSysMemMesh;
    if( pTempLocalMesh )  m_pLocalMesh  = pTempLocalMesh;

    // Compute normals if they are being requested and
    // the old mesh does not have them.
    if( !(dwOldFVF & D3DFVF_NORMAL) && dwFVF & D3DFVF_NORMAL )
    {
        if( m_pSysMemMesh )
            D3DXComputeNormals( m_pSysMemMesh, NULL );
        if( m_pLocalMesh )
            D3DXComputeNormals( m_pLocalMesh, NULL );
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: CDXUTMesh::SetVertexDecl
// Desc: Convert the mesh to the format specified by the given vertex
//       declarations.
//-----------------------------------------------------------------------------
HRESULT CDXUTMesh::SetVertexDecl( LPDIRECT3DDEVICE9 pd3dDevice, const D3DVERTEXELEMENT9 *pDecl )
{
    LPD3DXMESH pTempSysMemMesh = NULL;
    LPD3DXMESH pTempLocalMesh  = NULL;

    if( m_pSysMemMesh )
    {
        if( FAILED( m_pSysMemMesh->CloneMesh( m_pSysMemMesh->GetOptions(), pDecl,
                                              pd3dDevice, &pTempSysMemMesh ) ) )
            return E_FAIL;
    }

    if( m_pLocalMesh )
    {
        if( FAILED( m_pLocalMesh->CloneMesh( m_pLocalMesh->GetOptions(), pDecl, pd3dDevice,
                                             &pTempLocalMesh ) ) )
        {
            SAFE_RELEASE( pTempSysMemMesh );
            return E_FAIL;
        }
    }

    // Check if the old declaration contains a normal.
    bool bHadNormal = false;
    D3DVERTEXELEMENT9 aOldDecl[MAX_FVF_DECL_SIZE];
    if( m_pSysMemMesh && SUCCEEDED( m_pSysMemMesh->GetDeclaration( aOldDecl ) ) )
    {
        for( UINT index = 0; index < D3DXGetDeclLength( aOldDecl ); ++index )
            if( aOldDecl[index].Usage == D3DDECLUSAGE_NORMAL )
            {
                bHadNormal = true;
                break;
            }
    }

    // Check if the new declaration contains a normal.
    bool bHaveNormalNow = false;
    D3DVERTEXELEMENT9 aNewDecl[MAX_FVF_DECL_SIZE];
    if( pTempSysMemMesh && SUCCEEDED( pTempSysMemMesh->GetDeclaration( aNewDecl ) ) )
    {
        for( UINT index = 0; index < D3DXGetDeclLength( aNewDecl ); ++index )
            if( aNewDecl[index].Usage == D3DDECLUSAGE_NORMAL )
            {
                bHaveNormalNow = true;
                break;
            }
    }

    SAFE_RELEASE( m_pSysMemMesh );
    SAFE_RELEASE( m_pLocalMesh );

    if( pTempSysMemMesh )
    {
        m_pSysMemMesh = pTempSysMemMesh;

        if( !bHadNormal && bHaveNormalNow )
        {
            // Compute normals in case the meshes have them
            D3DXComputeNormals( m_pSysMemMesh, NULL );
        }
    }

    if( pTempLocalMesh )
    {
        m_pLocalMesh = pTempLocalMesh;

        if( !bHadNormal && bHaveNormalNow )
        {
            // Compute normals in case the meshes have them
            D3DXComputeNormals( m_pLocalMesh, NULL );
        }
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
HRESULT CDXUTMesh::RestoreDeviceObjects( LPDIRECT3DDEVICE9 pd3dDevice )
{
    if( NULL == m_pSysMemMesh )
        return E_FAIL;

    // Make a local memory version of the mesh. Note: because we are passing in
    // no flags, the default behavior is to clone into local memory.
    if( FAILED( m_pSysMemMesh->CloneMeshFVF( D3DXMESH_MANAGED | ( m_pSysMemMesh->GetOptions() & ~D3DXMESH_SYSTEMMEM ),
                                             m_pSysMemMesh->GetFVF(), pd3dDevice, &m_pLocalMesh ) ) )
        return E_FAIL;

    return S_OK;

}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
HRESULT CDXUTMesh::InvalidateDeviceObjects()
{
    SAFE_RELEASE( m_pLocalMesh );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
HRESULT CDXUTMesh::Destroy()
{
    InvalidateDeviceObjects();
    for( UINT i=0; i<m_dwNumMaterials; i++ )
        SAFE_RELEASE( m_pTextures[i] );
    SAFE_DELETE_ARRAY( m_pTextures );
    SAFE_DELETE_ARRAY( m_pMaterials );

    SAFE_RELEASE( m_pSysMemMesh );

    m_dwNumMaterials = 0L;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
HRESULT CDXUTMesh::Render( LPDIRECT3DDEVICE9 pd3dDevice, bool bDrawOpaqueSubsets,
                          bool bDrawAlphaSubsets )
{
    if( NULL == m_pLocalMesh )
        return E_FAIL;

    // Frist, draw the subsets without alpha
    if( bDrawOpaqueSubsets )
    {
        for( DWORD i=0; i<m_dwNumMaterials; i++ )
        {
            if( m_bUseMaterials )
            {
                if( m_pMaterials[i].Diffuse.a < 1.0f )
                    continue;
                pd3dDevice->SetMaterial( &m_pMaterials[i] );
                pd3dDevice->SetTexture( 0, m_pTextures[i] );
            }
            m_pLocalMesh->DrawSubset( i );
        }
    }

    // Then, draw the subsets with alpha
    if( bDrawAlphaSubsets && m_bUseMaterials )
    {
        for( DWORD i=0; i<m_dwNumMaterials; i++ )
        {
            if( m_pMaterials[i].Diffuse.a == 1.0f )
                continue;

            // Set the material and texture
            pd3dDevice->SetMaterial( &m_pMaterials[i] );
            pd3dDevice->SetTexture( 0, m_pTextures[i] );
            m_pLocalMesh->DrawSubset( i );
        }
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
HRESULT CDXUTMesh::Render( ID3DXEffect *pEffect,
                           D3DXHANDLE hTexture,
                           D3DXHANDLE hDiffuse,
                           D3DXHANDLE hAmbient,
                           D3DXHANDLE hSpecular,
                           D3DXHANDLE hEmissive,
                           D3DXHANDLE hPower,
                           bool bDrawOpaqueSubsets,
                           bool bDrawAlphaSubsets )
{
    if( NULL == m_pLocalMesh )
        return E_FAIL;

    UINT cPasses;
    // Frist, draw the subsets without alpha
    if( bDrawOpaqueSubsets )
    {
        pEffect->Begin( &cPasses, 0 );
        for( UINT p = 0; p < cPasses; ++p )
        {
            pEffect->BeginPass( p );
            for( DWORD i=0; i<m_dwNumMaterials; i++ )
            {
                if( m_bUseMaterials )
                {
                    if( m_pMaterials[i].Diffuse.a < 1.0f )
                        continue;
                    if( hTexture )
                        pEffect->SetTexture( hTexture, m_pTextures[i] );
                    // D3DCOLORVALUE and D3DXVECTOR4 are data-wise identical.
                    // No conversion is needed.
                    if( hDiffuse )
                        pEffect->SetVector( hDiffuse, (D3DXVECTOR4*)&m_pMaterials[i].Diffuse );
                    if( hAmbient )
                        pEffect->SetVector( hAmbient, (D3DXVECTOR4*)&m_pMaterials[i].Ambient );
                    if( hSpecular )
                        pEffect->SetVector( hSpecular, (D3DXVECTOR4*)&m_pMaterials[i].Specular );
                    if( hEmissive )
                        pEffect->SetVector( hEmissive, (D3DXVECTOR4*)&m_pMaterials[i].Emissive );
                    if( hPower )
                        pEffect->SetVector( hPower, (D3DXVECTOR4*)&m_pMaterials[i].Power );
                    pEffect->CommitChanges();
                }
                m_pLocalMesh->DrawSubset( i );
            }
            pEffect->EndPass();
        }
        pEffect->End();
    }

    // Then, draw the subsets with alpha
    if( bDrawAlphaSubsets && m_bUseMaterials )
    {
        pEffect->Begin( &cPasses, 0 );
        for( UINT p = 0; p < cPasses; ++p )
        {
            pEffect->BeginPass( p );
            for( DWORD i=0; i<m_dwNumMaterials; i++ )
            {
                if( m_bUseMaterials )
                {
                    if( m_pMaterials[i].Diffuse.a == 1.0f )
                        continue;
                    if( hTexture )
                        pEffect->SetTexture( hTexture, m_pTextures[i] );
                    // D3DCOLORVALUE and D3DXVECTOR4 are data-wise identical.
                    // No conversion is needed.
                    if( hDiffuse )
                        pEffect->SetVector( hDiffuse, (D3DXVECTOR4*)&m_pMaterials[i].Diffuse );
                    if( hAmbient )
                        pEffect->SetVector( hAmbient, (D3DXVECTOR4*)&m_pMaterials[i].Ambient );
                    if( hSpecular )
                        pEffect->SetVector( hSpecular, (D3DXVECTOR4*)&m_pMaterials[i].Specular );
                    if( hEmissive )
                        pEffect->SetVector( hEmissive, (D3DXVECTOR4*)&m_pMaterials[i].Emissive );
                    if( hPower )
                        pEffect->SetVector( hPower, (D3DXVECTOR4*)&m_pMaterials[i].Power );
                    pEffect->CommitChanges();
                }
                m_pLocalMesh->DrawSubset( i );
            }
            pEffect->EndPass();
        }
        pEffect->End();
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
CDXUTMeshFrame::CDXUTMeshFrame( LPCTSTR strName )
{
    StringCchCopy( m_strName, 512, strName );
    D3DXMatrixIdentity( &m_mat );
    m_pMesh  = NULL;

    m_pChild = NULL;
    m_pNext  = NULL;
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
CDXUTMeshFrame::~CDXUTMeshFrame()
{
    SAFE_DELETE( m_pChild );
    SAFE_DELETE( m_pNext );
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
bool CDXUTMeshFrame::EnumMeshes( bool (*EnumMeshCB)(CDXUTMesh*,void*),
                            void* pContext )
{
    if( m_pMesh )
        EnumMeshCB( m_pMesh, pContext );
    if( m_pChild )
        m_pChild->EnumMeshes( EnumMeshCB, pContext );
    if( m_pNext )
        m_pNext->EnumMeshes( EnumMeshCB, pContext );

    return TRUE;
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
CDXUTMesh* CDXUTMeshFrame::FindMesh( LPCTSTR strMeshName )
{
    CDXUTMesh* pMesh;

    if( m_pMesh )
        if( !lstrcmpi( m_pMesh->m_strName, strMeshName ) )
            return m_pMesh;

    if( m_pChild )
        if( NULL != ( pMesh = m_pChild->FindMesh( strMeshName ) ) )
            return pMesh;

    if( m_pNext )
        if( NULL != ( pMesh = m_pNext->FindMesh( strMeshName ) ) )
            return pMesh;

    return NULL;
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
CDXUTMeshFrame* CDXUTMeshFrame::FindFrame( LPCTSTR strFrameName )
{
    CDXUTMeshFrame* pFrame;

    if( !lstrcmpi( m_strName, strFrameName ) )
        return this;

    if( m_pChild )
        if( NULL != ( pFrame = m_pChild->FindFrame( strFrameName ) ) )
            return pFrame;

    if( m_pNext )
        if( NULL != ( pFrame = m_pNext->FindFrame( strFrameName ) ) )
            return pFrame;

    return NULL;
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
HRESULT CDXUTMeshFrame::Destroy()
{
    if( m_pMesh )  m_pMesh->Destroy();
    if( m_pChild ) m_pChild->Destroy();
    if( m_pNext )  m_pNext->Destroy();

    SAFE_DELETE( m_pMesh );
    SAFE_DELETE( m_pNext );
    SAFE_DELETE( m_pChild );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
HRESULT CDXUTMeshFrame::RestoreDeviceObjects( LPDIRECT3DDEVICE9 pd3dDevice )
{
    if( m_pMesh )  m_pMesh->RestoreDeviceObjects( pd3dDevice );
    if( m_pChild ) m_pChild->RestoreDeviceObjects( pd3dDevice );
    if( m_pNext )  m_pNext->RestoreDeviceObjects( pd3dDevice );
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
HRESULT CDXUTMeshFrame::InvalidateDeviceObjects()
{
    if( m_pMesh )  m_pMesh->InvalidateDeviceObjects();
    if( m_pChild ) m_pChild->InvalidateDeviceObjects();
    if( m_pNext )  m_pNext->InvalidateDeviceObjects();
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
HRESULT CDXUTMeshFrame::Render( LPDIRECT3DDEVICE9 pd3dDevice, bool bDrawOpaqueSubsets,
                           bool bDrawAlphaSubsets, D3DXMATRIX* pmatWorldMatrix )
{
    // For pure devices, specify the world transform. If the world transform is not
    // specified on pure devices, this function will fail.

    D3DXMATRIX matSavedWorld, matWorld;

    if ( NULL == pmatWorldMatrix )
        pd3dDevice->GetTransform( D3DTS_WORLD, &matSavedWorld );
    else
        matSavedWorld = *pmatWorldMatrix;

    D3DXMatrixMultiply( &matWorld, &m_mat, &matSavedWorld );
    pd3dDevice->SetTransform( D3DTS_WORLD, &matWorld );

    if( m_pMesh )
        m_pMesh->Render( pd3dDevice, bDrawOpaqueSubsets, bDrawAlphaSubsets );

    if( m_pChild )
        m_pChild->Render( pd3dDevice, bDrawOpaqueSubsets, bDrawAlphaSubsets, &matWorld );

    pd3dDevice->SetTransform( D3DTS_WORLD, &matSavedWorld );

    if( m_pNext )
        m_pNext->Render( pd3dDevice, bDrawOpaqueSubsets, bDrawAlphaSubsets, &matSavedWorld );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
HRESULT CDXUTMeshFile::LoadFrame( LPDIRECT3DDEVICE9 pd3dDevice,
                             LPD3DXFILEDATA pFileData,
                             CDXUTMeshFrame* pParentFrame )
{
    LPD3DXFILEDATA   pChildData = NULL;
    GUID Guid;
    SIZE_T      cbSize;
    CDXUTMeshFrame*  pCurrentFrame;
    HRESULT     hr;

    // Get the type of the object
    if( FAILED( hr = pFileData->GetType( &Guid ) ) )
        return hr;

    if( Guid == TID_D3DRMMesh )
    {
        hr = LoadMesh( pd3dDevice, pFileData, pParentFrame );
        if( FAILED(hr) )
            return hr;
    }
    if( Guid == TID_D3DRMFrameTransformMatrix )
    {
        D3DXMATRIX* pmatMatrix;
        hr = pFileData->Lock(&cbSize, (LPCVOID*)&pmatMatrix );
        if( FAILED(hr) )
            return hr;

        // Update the parent's matrix with the new one
        pParentFrame->SetMatrix( pmatMatrix );
    }
    if( Guid == TID_D3DRMFrame )
    {
        // Get the frame name
        CHAR  strAnsiName[512] = "";
        TCHAR strName[512];
        SIZE_T dwNameLength = 512;
        SIZE_T cChildren;
        if( FAILED( hr = pFileData->GetName( strAnsiName, &dwNameLength ) ) )
            return hr;

        strcpy(strAnsiName, strName);
        strName[511] = 0;

        // Create the frame
        pCurrentFrame = new CDXUTMeshFrame( strName );
        if( pCurrentFrame == NULL )
            return E_OUTOFMEMORY;

        pCurrentFrame->m_pNext = pParentFrame->m_pChild;
        pParentFrame->m_pChild = pCurrentFrame;

        // Enumerate child objects
        pFileData->GetChildren(&cChildren);
        for (UINT iChild = 0; iChild < cChildren; iChild++)
        {
            // Query the child for its FileData
            hr = pFileData->GetChild(iChild, &pChildData );
            if( SUCCEEDED(hr) )
            {
                hr = LoadFrame( pd3dDevice, pChildData, pCurrentFrame );
                SAFE_RELEASE( pChildData );
            }

            if( FAILED(hr) )
                return hr;
        }
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
HRESULT CDXUTMeshFile::LoadMesh( LPDIRECT3DDEVICE9 pd3dDevice,
                            LPD3DXFILEDATA pFileData,
                            CDXUTMeshFrame* pParentFrame )
{
    // Currently only allowing one mesh per frame
    if( pParentFrame->m_pMesh )
        return E_FAIL;

    // Get the mesh name
    CHAR  strAnsiName[512] = {0};
    TCHAR strName[512];
    SIZE_T dwNameLength = 512;
    HRESULT hr;
    if( FAILED( hr = pFileData->GetName( strAnsiName, &dwNameLength ) ) )
        return hr;

    strcpy(strName, strAnsiName);
    strName[511] = 0;

    // Create the mesh
    pParentFrame->m_pMesh = new CDXUTMesh( strName );
    if( pParentFrame->m_pMesh == NULL )
        return E_OUTOFMEMORY;
    pParentFrame->m_pMesh->Create( pd3dDevice, pFileData );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
HRESULT CDXUTMeshFile::CreateFromResource( LPDIRECT3DDEVICE9 pd3dDevice, LPCTSTR strResource, LPCTSTR strType )
{
    LPD3DXFILE           pDXFile   = NULL;
    LPD3DXFILEENUMOBJECT pEnumObj  = NULL;
    LPD3DXFILEDATA       pFileData = NULL;
    HRESULT hr;
    SIZE_T cChildren;

    // Create a x file object
    if( FAILED( hr = D3DXFileCreate( &pDXFile ) ) )
        return E_FAIL;

    // Register templates for d3drm and patch extensions.
    if( FAILED( hr = pDXFile->RegisterTemplates( (void*)D3DRM_XTEMPLATES,
                                                 D3DRM_XTEMPLATE_BYTES ) ) )
    {
        SAFE_RELEASE( pDXFile );
        return E_FAIL;
    }
    
    CHAR strTypeAnsi[MAX_PATH];
    CHAR strResourceAnsi[MAX_PATH];

    strcpy(strTypeAnsi, strType);
    strTypeAnsi[MAX_PATH-1] = 0;

    strcpy(strResourceAnsi, strResource);
    strResourceAnsi[MAX_PATH-1] = 0;

    D3DXF_FILELOADRESOURCE dxlr;
    dxlr.hModule = NULL;
    dxlr.lpName = strResourceAnsi;
    dxlr.lpType = strTypeAnsi;

    // Create enum object
    hr = pDXFile->CreateEnumObject( (void*)&dxlr, D3DXF_FILELOAD_FROMRESOURCE, 
                                    &pEnumObj );
    if( FAILED(hr) )
    {
        SAFE_RELEASE( pDXFile );
        return hr;
    }

    // Enumerate top level objects (which are always frames)
    pEnumObj->GetChildren(&cChildren);
    for (UINT iChild = 0; iChild < cChildren; iChild++)
    {
        hr = pEnumObj->GetChild(iChild, &pFileData);
        if (FAILED(hr))
            return hr;

        hr = LoadFrame( pd3dDevice, pFileData, this );
        SAFE_RELEASE( pFileData );
        if( FAILED(hr) )
        {
            SAFE_RELEASE( pEnumObj );
            SAFE_RELEASE( pDXFile );
            return E_FAIL;
        }
    }

    SAFE_RELEASE( pFileData );
    SAFE_RELEASE( pEnumObj );
    SAFE_RELEASE( pDXFile );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
HRESULT CDXUTMeshFile::Create( LPDIRECT3DDEVICE9 pd3dDevice, LPCTSTR strFilename )
{
    LPD3DXFILE           pDXFile   = NULL;
    LPD3DXFILEENUMOBJECT pEnumObj  = NULL;
    LPD3DXFILEDATA       pFileData = NULL;
    HRESULT hr;
    SIZE_T cChildren;

    // Create a x file object
    if( FAILED( hr = D3DXFileCreate( &pDXFile ) ) )
        return E_FAIL;

    // Register templates for d3drm and patch extensions.
    if( FAILED( hr = pDXFile->RegisterTemplates( (void*)D3DRM_XTEMPLATES,
                                                 D3DRM_XTEMPLATE_BYTES ) ) )
    {
        SAFE_RELEASE( pDXFile );
        return E_FAIL;
    }

    // Find the path to the file, and convert it to ANSI (for the D3DXOF API)
    TCHAR strPath[MAX_PATH];
    CHAR  strPathANSI[MAX_PATH];
    DXUTFindDXSDKMediaFileCch( strPath, sizeof(strPath) / sizeof(TCHAR), strFilename );
    
	strcpy(strPathANSI,strPath);
    strPathANSI[MAX_PATH-1] = 0;
    
    // Create enum object
    hr = pDXFile->CreateEnumObject( (void*)strPathANSI, D3DXF_FILELOAD_FROMFILE, 
                                    &pEnumObj );
    if( FAILED(hr) )
    {
        SAFE_RELEASE( pDXFile );
        return hr;
    }

    // Enumerate top level objects (which are always frames)
    pEnumObj->GetChildren(&cChildren);
    for (UINT iChild = 0; iChild < cChildren; iChild++)
    {
        hr = pEnumObj->GetChild(iChild, &pFileData);
        if (FAILED(hr))
            return hr;

        hr = LoadFrame( pd3dDevice, pFileData, this );
        SAFE_RELEASE( pFileData );
        if( FAILED(hr) )
        {
            SAFE_RELEASE( pEnumObj );
            SAFE_RELEASE( pDXFile );
            return E_FAIL;
        }
    }

    SAFE_RELEASE( pFileData );
    SAFE_RELEASE( pEnumObj );
    SAFE_RELEASE( pDXFile );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name:
// Desc:
//-----------------------------------------------------------------------------
HRESULT CDXUTMeshFile::Render( LPDIRECT3DDEVICE9 pd3dDevice, D3DXMATRIX* pmatWorldMatrix )
{

    // For pure devices, specify the world transform. If the world transform is not
    // specified on pure devices, this function will fail.

    // Set up the world transformation
    D3DXMATRIX matSavedWorld, matWorld;

    if ( NULL == pmatWorldMatrix )
        pd3dDevice->GetTransform( D3DTS_WORLD, &matSavedWorld );
    else
        matSavedWorld = *pmatWorldMatrix;

    D3DXMatrixMultiply( &matWorld, &matSavedWorld, &m_mat );
    pd3dDevice->SetTransform( D3DTS_WORLD, &matWorld );

    // Render opaque subsets in the meshes
    if( m_pChild )
        m_pChild->Render( pd3dDevice, TRUE, FALSE, &matWorld );

    // Enable alpha blending
    pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
    pd3dDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
    pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );

    // Render alpha subsets in the meshes
    if( m_pChild )
        m_pChild->Render( pd3dDevice, FALSE, TRUE, &matWorld );

    // Restore state
    pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
    pd3dDevice->SetTransform( D3DTS_WORLD, &matSavedWorld );

    return S_OK;
}




