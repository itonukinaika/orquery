// orquery ORCA patient info query tool by Itonuki Naika Clinic
// 日医標準レセプトソフトデータベーステーブル定義書
// https://www.orca.med.or.jp/receipt/tec/database_table_definition_edition.html
// に従って、患者番号から氏名などを検索して標準出力に書き出す
// licenced under GPL v3

#include <stdio.h>
#include <windows.h>
#include <libpq-fe.h>
#include <iconv.h>

char server[64] = "";
char password[64] = "";
char conninfo[256] = "";
char sqlcommand[1024] = "";

PGconn     *conn;
PGresult   *res;

iconv_t cd;
size_t length;
char * pIn;
char * pOut;

char PTNum[7]="";
char FormattedPTNum[21]="";
char PTID[11]="";
char PTNameUTF8[401]="";
char PTNameSJIS[201]="";
char PTSex[2]="";
char PTBirthday[9]="";
char PTHomeTEL1[61]="";
char PTHomeTEL2[61]="";

int WINAPI WinMain(HINSTANCE hInstance , HINSTANCE hPrevInstance , PSTR lpCmdLine , int nCmdShow ) {
	
	int i=0;
	for (i = 0; i < __argc; i++) {
		switch(i){
			case 1:
				//第1引数として患者番号
				strncpy(PTNum, __argv[1], 6);
				break;
			case 2:
				//第2引数としてORCAサーバのアドレス
				strncpy(server, __argv[2], 63);
				break;
			case 3:
				//第3引数としてORCAデータベースのパスワード（設定されていれば）
				strncpy(password, __argv[3], 63);
				break;
		}
	}
	
	//PQconnectDB用に、ホスト名、データベース名等の文字列を生成する。
	strcpy(conninfo,"host=");
	strcat(conninfo,server);
	//データベース名、ユーザー名が異なる場合は、下記を修正すること。
	strcat(conninfo," dbname=orca user=orca");
	//パスワードを引数として指定すれば、文字列にパスワードも追加する。環境によって異なる。
	if (strlen(password)>0){
		strcat(conninfo," password=");
		strcat(conninfo,password);
	}
	
	//ORCAに接続する
	conn = PQconnectdb(conninfo);
	if (PQstatus(conn) != CONNECTION_OK)
    {
    	//OCRAに接続できなければ、エラーを表示して終了する
    	printf("DATABASE CONNECTION FAILED: %s", PQerrorMessage(conn));
        PQfinish(conn);
    	return 1;
    }
	
	//まず、引数として与えられた患者番号から、ORCAデータベース内の患者番号フォーマット（tbl_ptnumにおけるptnum）に変換する
	//ORCA内部の患者番号（ptid）は、この番号とは必ずしも一致しない
	//ptnumのフォーマットは医療機関ごとに異なる。
	//一例として、下記で6桁の数値＋14桁の空白としているが、自院のデータベースを参照して、適宜変更すること。
	sprintf(FormattedPTNum, "%06d              ", atoi(PTNum));
	
	//tbl_ptnumを検索して、ORCA内部のptidへと変換する
	strcpy(sqlcommand,"SELECT ptid FROM tbl_ptnum WHERE ptnum = \'");
	strcat(sqlcommand,FormattedPTNum);
	strcat(sqlcommand,"\'");
	res = PQexec(conn, sqlcommand);
	//検索できなければ、エラーを表示して終了する
	if ((PQresultStatus(res) != PGRES_TUPLES_OK)||(PQntuples(res) == 0))
    {
		printf("PTID QUERY FAILED: %s", PQerrorMessage(conn));
		PQclear(res);
    	PQfinish(conn);
    	return 1;
    }
	//PTIDは10桁のbigintと定義されている。以降のクエリのため、10桁の整数文字列に変換する。
	sprintf(PTID, "%010d", atoi(PQgetvalue(res, 0, 0)));
	PQclear(res);
	
	//tbl_ptinfを検索して、患者氏名（UTF-8エンコード）を所得する
	strcpy(sqlcommand,"SELECT name FROM tbl_ptinf WHERE ptid = \'");
	strcat(sqlcommand,PTID);
	strcat(sqlcommand,"\'");
	res = PQexec(conn, sqlcommand);
	strncpy(PTNameUTF8, PQgetvalue(res, 0, 0), 400);
	PQclear(res);
	
	//iconvライブラリを用いて、患者氏名をSJISに変換する（他の文字コードを用いる場合は、以下を適宜変更する）
	//ORCAデータベースの文字コードがEUC-JPであれば、UTF-8のところをEUC-JPに変更すればよい
	cd = iconv_open ("SHIFT-JIS", "UTF-8");
	length=strlen(PTNameUTF8);
	pIn = PTNameUTF8;
	pOut = (char*)PTNameSJIS;
	iconv(cd, &pIn, &length, &pOut, &length);
	
	//tbl_ptinfを検索して、性別コードを所得する
	strcpy(sqlcommand,"SELECT sex FROM tbl_ptinf WHERE ptid = \'");
	strcat(sqlcommand,PTID);
	strcat(sqlcommand,"\'");
	res = PQexec(conn, sqlcommand);
	strncpy(PTSex, PQgetvalue(res, 0, 0), 1);
	PQclear(res);
	
	//誕生日
	strcpy(sqlcommand,"SELECT birthday FROM tbl_ptinf WHERE ptid = \'");
	strcat(sqlcommand,PTID);
	strcat(sqlcommand,"\'");
	res = PQexec(conn, sqlcommand);
	strncpy(PTBirthday, PQgetvalue(res, 0, 0), 8);
	PQclear(res);
	
	//連絡先1
	strcpy(sqlcommand,"SELECT home_tel1 FROM tbl_ptinf WHERE ptid = \'");
	strcat(sqlcommand,PTID);
	strcat(sqlcommand,"\'");
	res = PQexec(conn, sqlcommand);
	strncpy(PTHomeTEL1, PQgetvalue(res, 0, 0), 60);
	PQclear(res);
	
	//連絡先2
	strcpy(sqlcommand,"SELECT home_tel2 FROM tbl_ptinf WHERE ptid = \'");
	strcat(sqlcommand,PTID);
	strcat(sqlcommand,"\'");
	res = PQexec(conn, sqlcommand);
	strncpy(PTHomeTEL2, PQgetvalue(res, 0, 0), 60);
	PQclear(res);
	
	//標準出力に書き出す
	//連携するアプリケーションに適した内容、順序に適宜変更して用いる
	printf("%s\n%s\n%s\n%s\n%s\n",PTNameSJIS,PTSex,PTBirthday,PTHomeTEL1,PTHomeTEL2);
	
	//ORCAから切断する
	PQfinish(conn);
	return 0;
}