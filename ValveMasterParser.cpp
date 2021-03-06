// ValveMasterParser.cpp: ���������� ����� ����� ��� ����������� ����������.
//

#include "stdafx.h"
#include "Csocket.h"
#include "DataParser.h"
#include <process.h>

void GameDataLog( char *fmt, ... )
{
	char string[4096];
	memset( string, 0, sizeof(string) );

	va_list marker;
	va_start( marker, fmt );
	vsprintf( string + strlen(string), fmt, marker );
	va_end( marker );

	FILE *fp = fopen( "serverinfo.log", "ab" );
	fprintf( fp, "%s", string );
	fclose( fp );
}

#pragma pack( push, 1 )

struct servadr_t
{
	unsigned long ip;
	unsigned short port;
};

#pragma pack( pop )

struct serverinfo_t
{
	int  serverport;
	char pServAddr[1024];

	byte protocol;
	char pHostName[1024];
	char pMapName[1024];
	char pGameDir[1024];
	char pGameName[1024];
	short appid;

	byte curpl;
	byte maxplayers;
	byte bots;
	byte type;
	byte os;
	byte password;

	//Mod info for old servers
	byte ismod;
	char pLinkMod[1024];
	char pDownloadLink[1024];
	long modversion;
	long modsize;
	byte typemod;
	byte isuseowndll;

	byte secure;

	char pVersion[1024];
	short gameport;
	long long steamid;
	short sourcetvport;
	char pSourceTVName[1024];
	char tags[1024];
	long long gameid;
};

int serverid = 0;

int GetServerInfo( char *pServerIP, int port )
{
	serverinfo_t pServerInfo;
	memset( &pServerInfo, 0, sizeof(pServerInfo) );

	strcpy( pServerInfo.pServAddr, pServerIP);
	pServerInfo.serverport = port;
	
	Csocket *pSocket = new Csocket(eSocketProtocolUDP);
	pSocket->SetAdr( pServerIP, port );
	pSocket->SetTimeOut( 500000 );

	char pQueryPack[512];
	memset(pQueryPack, 0, sizeof(pQueryPack) );
	sprintf( pQueryPack, "\xFF\xFF\xFF\xFFTSource Engine Query" );
	pSocket->Send( (unsigned char *)pQueryPack, strlen(pQueryPack) + 1 );

	unsigned char pRecvBuff2[10240];
	memset(pRecvBuff2, 0, sizeof(pRecvBuff2));
	int recvbytes = pSocket->Recv(pRecvBuff2, sizeof(pRecvBuff2) );
	delete pSocket;

	if( pRecvBuff2[0] == 0xFF && pRecvBuff2[1] == 0xFF && pRecvBuff2[2] == 0xFF && pRecvBuff2[3] == 0xFF && pRecvBuff2[4] == 0x49 ) //Current default query
	{
		CDataParser *pDataParser = new CDataParser( pRecvBuff2, recvbytes);

		//Move headers
		pDataParser->MoveOffset(5);

		pServerInfo.protocol = pDataParser->GetByte();
		sprintf( pServerInfo.pHostName, "%s", pDataParser->GetString() );
		sprintf( pServerInfo.pMapName, "%s", pDataParser->GetString() );
		sprintf( pServerInfo.pGameDir, "%s", pDataParser->GetString() );
		sprintf( pServerInfo.pGameName, "%s", pDataParser->GetString() );

		pServerInfo.appid =  pDataParser->GetShort();
		pServerInfo.curpl = pDataParser->GetByte();
		pServerInfo.maxplayers = pDataParser->GetByte();
		pServerInfo.bots = pDataParser->GetByte();
		pServerInfo.type = pDataParser->GetByte();
		pServerInfo.os = pDataParser->GetByte();
		pServerInfo.password = pDataParser->GetByte();
		pServerInfo.secure = pDataParser->GetByte();

		sprintf( pServerInfo.pVersion, "%s", pDataParser->GetString() );

		if( pDataParser->GetOffset() < recvbytes )
		{
			byte EDF = pDataParser->GetByte();

			if( EDF & 0x80 )
			{
				pServerInfo.gameport =  pDataParser->GetShort();
			}

			if( EDF & 0x10 )
			{
				pServerInfo.steamid =  pDataParser->GetLongLong();
			}

			if ( EDF & 0x40 )
			{
				pServerInfo.sourcetvport =  pDataParser->GetShort();
				sprintf( pServerInfo.pSourceTVName, "%s", pDataParser->GetString() );
			}

			if ( EDF & 0x20 )
			{
				sprintf( pServerInfo.tags, "%s", pDataParser->GetString() );
			}

			if ( EDF & 0x01 )
			{
				pServerInfo.gameid =  pDataParser->GetLongLong();
			}
		}

		delete pDataParser;
	}
	else if( pRecvBuff2[0] == 0xFF && pRecvBuff2[1] == 0xFF && pRecvBuff2[2] == 0xFF && pRecvBuff2[3] == 0xFF && pRecvBuff2[4] == 0x6D ) //Old Protocol
	{
		CDataParser *pDataParser = new CDataParser( pRecvBuff2, recvbytes);

		//Move headers
		pDataParser->MoveOffset(5);

		//First send local ip addr, not interesting
		pDataParser->GetString();

		sprintf( pServerInfo.pHostName, "%s", pDataParser->GetString() );
		sprintf( pServerInfo.pMapName, "%s", pDataParser->GetString() );
		sprintf( pServerInfo.pGameDir, "%s", pDataParser->GetString() );
		sprintf( pServerInfo.pGameName, "%s", pDataParser->GetString() );

		pServerInfo.curpl = pDataParser->GetByte();
		pServerInfo.maxplayers = pDataParser->GetByte();
		pServerInfo.protocol = pDataParser->GetByte();
		pServerInfo.type = pDataParser->GetByte();
		pServerInfo.os = pDataParser->GetByte();
		pServerInfo.password = pDataParser->GetByte();

		//Only for old
		pServerInfo.ismod = pDataParser->GetByte();

		if(pServerInfo.ismod)
		{
			sprintf( pServerInfo.pLinkMod, "%s", pDataParser->GetString() );
			sprintf( pServerInfo.pDownloadLink, "%s", pDataParser->GetString() );

			pDataParser->GetByte(); //unused null byte

			pServerInfo.modversion = pDataParser->GetLong();
			pServerInfo.modsize = pDataParser->GetLong();
			pServerInfo.typemod = pDataParser->GetByte();
			pServerInfo.isuseowndll = pDataParser->GetByte();
		}

		pServerInfo.secure = pDataParser->GetByte();
		pServerInfo.bots = pDataParser->GetByte();

		delete pDataParser;
	}
	else
	{
		return 0;
	}

/*
	GameDataLog( "ServerID: %d\r\n", serverid);
	GameDataLog( "Addr: %s:%d\r\n", pServerInfo.pServAddr, pServerInfo.serverport);
	GameDataLog( "Protocol version: %d\r\n", pServerInfo.protocol);
	GameDataLog( "Hostname: %s\r\n", pServerInfo.pHostName);
	GameDataLog( "Map: %s\r\n", pServerInfo.pMapName);
	GameDataLog( "GameDir: %s\r\n", pServerInfo.pGameDir);
	GameDataLog( "Name of game: %s\r\n", pServerInfo.pGameName);
	GameDataLog( "App Id: %d\r\n", pServerInfo.appid);

	GameDataLog( "Current players: %d\r\n", pServerInfo.curpl);
	GameDataLog( "Max players: %d\r\n", pServerInfo.maxplayers);
	GameDataLog( "Bots: %d\r\n", pServerInfo.bots);

	switch( pServerInfo.type )
	{
	case 'd':
			GameDataLog( "Type server: Dedicated\r\n");
		break;
	case 'l':
			GameDataLog( "Type server: Listern\r\n");
		break;
	case 'p':
			GameDataLog( "Type server:  SourceTV\r\n");
		break;
	}

	switch( pServerInfo.os )
	{
	case 'w':
			GameDataLog( "OS server: Windows\r\n");
		break;
	case 'l':
			GameDataLog( "OS server: Linux\r\n");
		break;
	case 'm':
			GameDataLog( "OS server: Mac\r\n");
		break;
	}

	switch( pServerInfo.password )
	{
	case 0:
			GameDataLog( "Password not set\r\n");
		break;
	case 1:
			GameDataLog( "Password set\r\n");
		break;
	}

	switch( pServerInfo.secure )
	{
	case 0:
			GameDataLog( "VAC disabled\r\n");
		break;
	case 1:
			GameDataLog( "VAC enabled\r\n");
		break;
	}

	switch( pServerInfo.ismod )
	{
	case 0:
		GameDataLog( "Type game: Original\r\n");
		break;
	case 1:
		GameDataLog( "Type game: Mod\r\n");
		break;
	}

	GameDataLog( "Link to mod: %s\r\n", pServerInfo.pLinkMod);
	GameDataLog( "Download link to mod: %s\r\n", pServerInfo.pDownloadLink);
	GameDataLog( "Mod verision: %d\r\n", pServerInfo.modversion);
	GameDataLog( "Mod size: %d\r\n", pServerInfo.modsize);

	switch( pServerInfo.typemod )
	{
	case 0:
		GameDataLog( "Type mod: single and multiplayer\r\n");
		break;
	case 1:
		GameDataLog( "Type mod: multiplayer only mod\r\n");
		break;
	}

	switch( pServerInfo.isuseowndll )
	{
	case 0:
		GameDataLog( "Mod use Half-Life DLL\r\n");
		break;
	case 1:
		GameDataLog( "Mod use own DLL\r\n");
		break;
	}

	GameDataLog( "Client version: %s\r\n", pServerInfo.pVersion);
	GameDataLog( "Game port: %d\r\n", pServerInfo.gameport);
	GameDataLog( "SteamID: %ld\r\n", pServerInfo.steamid);
	GameDataLog( "SourceTV port: %d\r\n", pServerInfo.sourcetvport);
	GameDataLog( "SourceTV name: %s\r\n", pServerInfo.pSourceTVName);
	GameDataLog( "Tags: %s\r\n", pServerInfo.tags);
	GameDataLog( "GameID: %ld\r\n", pServerInfo.gameid);
	GameDataLog( "\r\n\r\n" );
*/

	while(true)
	{
		FILE *fp = fopen( "list.csv", "ab" );

		if(fp) //while for free file for open ))
		{
			fprintf( fp, "%d;%s;%d;%d;%s;%s;%s;%s;%d;%d\n", serverid, pServerInfo.pServAddr, pServerInfo.serverport, pServerInfo.protocol, pServerInfo.pHostName, pServerInfo.pMapName, pServerInfo.pGameDir, pServerInfo.pGameName, pServerInfo.appid, pServerInfo.gameid);

			fclose( fp );
			break;
		}
	}

	char pDirectory[MAX_PATH];

	if( pServerInfo.gameid > 0 )
	{
		sprintf(pDirectory, "%d", pServerInfo.gameid );
	}
	else if( pServerInfo.appid > 0 )
	{
		sprintf(pDirectory, "%d", pServerInfo.appid );
	}
	else
	{
		sprintf(pDirectory, "%d", 0 );
	}

	CreateDirectory( pDirectory, NULL );

	strcat( pDirectory, "/" );
	strcat( pDirectory, pServerInfo.pGameDir );
	CreateDirectory( pDirectory, NULL );

	CreateDirectory( pDirectory, NULL );
	strcat( pDirectory, "/" );
	strcat( pDirectory, pServerInfo.pMapName );
	CreateDirectory( pDirectory, NULL );
	serverid++;

	return 1;
}

int globalServerId = 0;


struct threaddata_t
{
	int serverid;
	char pServerIP[512];
	int port;
};

void ThreadForGetServerInfo( void* pParams )
{
	threaddata_t *pThreadData = (threaddata_t *)pParams;

	GetServerInfo( pThreadData->pServerIP, pThreadData->port );

	free(pThreadData);
}

int QueryMasterServer(  Csocket *pSocket, char *pStartServerIP, int port, int AppId )
{
	char pQuery[1024];
	memset( pQuery, 0, sizeof(pQuery) );
	sprintf( pQuery, "\x31\xFF" );

	char pAddrQuery[256];
	memset( pAddrQuery, 0, sizeof(pAddrQuery) );
	sprintf( pAddrQuery, "%s:%d", pStartServerIP, port);
	strcat( pQuery, pAddrQuery );

	if(AppId > 0)
	{
		char pFilter[1024];
		memset( pFilter, 0, sizeof(pFilter) );
		sprintf( pFilter, "\\appid\\%d", AppId);

		strcpy( pQuery + strlen(pQuery) + 1, pFilter );

		pSocket->Send( (unsigned char *)pQuery, strlen(pQuery) + 1 + strlen(pFilter) + 1 );
	}
	else
	{
		pSocket->Send( (unsigned char *)pQuery, strlen(pQuery) + 1 );
	}

	char pRecvData[4096];
	memset( pRecvData, 0, sizeof(pRecvData) );

	int clrecvsize = pSocket->Recv( (unsigned char *)pRecvData, sizeof(pRecvData) );

	LogPrintf( false, "Get answer from server: %d bytes App: %d\r\n", clrecvsize, AppId );

	if( clrecvsize > 6 && pRecvData[0] == '\xFF' && pRecvData[1] == '\xFF' && pRecvData[2] == '\xFF' && pRecvData[3] == '\xFF' && pRecvData[4] == '\x66' && pRecvData[5] == '\x0A' /*&& (clrecvsize - 6) & 6*/ )
	{
		int countaddr = (clrecvsize / 6) - 1;

		LogPrintf( false, "Get corrected answer from: %d servers App: %d\r\n", (clrecvsize / 6) - 1, AppId ); 

		servadr_t *pIpAddrList = (servadr_t *)(pRecvData + 6);

		for( int i = 0; i < countaddr; i++ )
		{
			in_addr in;
			in.S_un.S_addr = pIpAddrList[i].ip;

			globalServerId++;

			LogPrintf( false, "Server %d: %s:%d App: %d\r\n", globalServerId, inet_ntoa( in ), htons( pIpAddrList[i].port ), AppId );

			threaddata_t *pThreadData = new threaddata_t();
			pThreadData->serverid = globalServerId;
			strcpy( pThreadData->pServerIP, inet_ntoa( in ) );
			pThreadData->port = htons( pIpAddrList[i].port);

			_beginthread( ThreadForGetServerInfo, 0, pThreadData );
		}

		in_addr in;
		in.S_un.S_addr = pIpAddrList[countaddr-1].ip;

		if( pIpAddrList[countaddr-1].ip != 0 && pIpAddrList[countaddr-1].port != 0 )
		{
			Sleep(3000);
			return QueryMasterServer( pSocket, inet_ntoa( in ), htons( pIpAddrList[countaddr-1].port), AppId );
		}
		else
		{
			return 1;
		}
	}
	else
	{
		delete pSocket;
		Sleep(5000);
		pSocket = new Csocket(eSocketProtocolUDP);

		//	List not oficial master servers: https://hlmaster.info/
		pSocket->SetAdr( "hl2master.steampowered.com", 27011 );

		pSocket->SetTimeOut( 500000 );

		return QueryMasterServer(  pSocket, pStartServerIP, port, AppId );
	}

	return 1;
}

// Another user is requesting a challenge value from this machine
// NOTE: this is currently duplicated in SteamClient.dll but for a different purpose,
// so these can safely diverge anytime. SteamClient will be using a different protocol
// to update the master servers anyway.
#define A2S_GETCHALLENGE		'q'	// Request challenge # from another machine


#define A2M_GET_SERVERS_BATCH2	'1' // New style server query
// Master response with server list for channel
#define M2A_SERVER_BATCH		'f' // + int32 next uniqueID( -1 for last batch ) + 6 byte IP/Port list.

#define	C2M_CHECKMD5			'M'	// player client asks secure master if Module MD5 is valid
#define M2C_ISVALIDMD5			'N'	// secure servers answer to C2M_CHECKMD5

// Generic Ping Request
#define	A2A_PING				'i'	// respond with an A2A_ACK

// Generic Ack
#define	A2A_ACK					'j'	// general acknowledgement without info

int main(int argc, char* argv[] )
{
//	LogPrintf(false, "Master server checker...\r\n" );
	printf( "Master server checker...\r\n" );

	Csocket *pSocket = new Csocket(eSocketProtocolUDP);

//	List not oficial master servers: https://hlmaster.info/
	pSocket->SetAdr( "hl2master.steampowered.com", 27011 );

	pSocket->SetTimeOut( 500000 );

//	for( int i = 731; i < 65000; i++ )
//	{
//		QueryMasterServer( pSocket, "0.0.0.0", 0, i );
//	}

	QueryMasterServer( pSocket, "0.0.0.0", 0, 0 );

/*
	while(true)
	{
		//GoldSRC mods
		QueryMasterServer( pSocket, "0.0.0.0", 0, 70 );

		//Source Engine mods
		QueryMasterServer( pSocket, "0.0.0.0", 0, 215 );
		QueryMasterServer( pSocket, "0.0.0.0", 0, 218 );
		QueryMasterServer( pSocket, "0.0.0.0", 0, 243750 );
	}
*/
	//All games
//	QueryMasterServer( pSocket, "0.0.0.0", 0, 0 );

	delete pSocket;

	return 1;
}

