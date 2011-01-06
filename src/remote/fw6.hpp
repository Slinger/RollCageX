/**

	File Author: Codie Morgan
	Using FW6 style commenting....
	
	non functional conceptual php interfacing file
	
	this will help me plan the rcx php modules for fw6 accordingly

**/

/*
	
	class would need some form of http file open wtapper to read parsed 
	
	result directly via http: for readonly mode
	
*/

#define FW_URL_BASE	'http:\/\/gorcx.net/fw6/'
#define FW_URL '?dataonly&rcx='
#define FW_STATFILE_URL 'natsume/data/rcx/' // -> domainroot relative

/**
	fw6 root class
		+ sub mysql interface ( optional, since remotly FW6 should have that abaility after recieveing data )
**/
class fw6
{
	private:
		int token,playerid;
		char player_username;
		bool isHost,isRacing,isSpectator,isLeagueOperator,isServerAdmin,isCustomFWHost;
		
	public:
		const char uri_get		=	'&get='; //data
		const char uri_set		=	'&set='; // data
		const char uri_token	=	'&token=';
		const char uri_rtoken	=	'&tokenrequest=';
		const char uri_command	=	'&command=';
		const char uri_player	=	'&p=';
		const char uri_master	=	'&'; // get IP of master server
		const char uri_slist_1	=	'&'; // send - master server ip - destination
		const char uri_slist_2	=	'&'; // send - master server ip - data
		const char uri_do_t		=	'&'; // set track so that server can note it's layout
		const char uri_pl		=	'&'; // submit player list + ips
		const char uri_do_race	=	'&'; // start race + server side lap counter
		const char uri_do_lap	=	'&'; // trigger that player has hit a lap
		const char uri_do_cp	=	'&'; // trigger that player has hit a checkpoint
		const char uri_bicode	=	'&'; // enables a unicode utf-8 offset 3step shift
		const char uri_bicode	=	'&'; // enables a unicode utf-8 offset 3step shift
		
		// ... etc....
	
	bool getToken(char player_username,char password,bool createAccountIfMissing)
	{
		// etc... 
	}
}