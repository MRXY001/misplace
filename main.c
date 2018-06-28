#include <stdio.h>
#include <stdlib.h>
#include <winsock.h>
#include <mysql.h>
#include <time.h>
#include <string.h>
#include "cgic.h"
#include "ctemplate.h"

#define MySQL_username "root"
#define MySQL_password "root"

int gotovim = 0;
char sharecode[100] = { 0 };
char nowtime[1024] = { 0 };

void executeNonQuery(char * sql);
MYSQL_RES * executeQuery(char * sql);
void saveinfo(char * name, char * info);
void buildRandShareCode();
void getShareCode(char * name);
void setNowTime();

TMPL_varlist * varlist1 = 0;
void prt_enter(char * msg) // 入口+提示
{
	varlist1 = TMPL_add_var(varlist1, "msg", msg, 0);
	TMPL_write("misenter.htm", 0, 0, varlist1, cgiOut, cgiOut);
}
void prt_editor(char * name, char * info) // 编辑器
{
	varlist1 = TMPL_add_var(varlist1, "name", name, "info", info, 0);
	if (!gotovim)
		TMPL_write("miseditor.htm", 0, 0, varlist1, cgiOut, cgiOut);
	else
		TMPL_write("misvim.htm", 0, 0, varlist1, cgiOut, cgiOut);
}
void prt_editor_msg(char * name, char * info, char * msg) // 编辑器+提示
{
	varlist1 = TMPL_add_var(varlist1, "name", name, "info", info, "msg", msg, 0);
	if (!gotovim)
		TMPL_write("miseditor.htm", 0, 0, varlist1, cgiOut, cgiOut);
	else
		TMPL_write("misvim.htm", 0, 0, varlist1, cgiOut, cgiOut);
}
void prt_code(char * str) // 代码
{
	varlist1 = TMPL_add_var(varlist1, "codetext", str, 0);
	TMPL_write("miscode.htm", 0, 0, varlist1, cgiOut, cgiOut);
}
void prt_coder(char * str, char * lang) // 代码+自定义语言
{
	varlist1 = TMPL_add_var(varlist1, "codetext", str, "codelang", lang, 0);
	//printf("lang=%s<br />", lang);
	TMPL_write("miscoder.htm", 0, 0, varlist1, cgiOut, cgiOut);
}
void prt_read(char * str) // 只读
{
	// 如果包含<HTML>或者<html>，则无视下面的
	// 如果第一行是<TITLE>或者<title>，则标题为这个，其余照常输出
	// 如果第一行是<URL>或者<url>，则重定向至另一个网站
	// 如果第一行是<CODE>或者<code>，则打开代码阅读
	// <CODE><lang>或者<code><lang>则打开代码阅读2（自定义语言）
	// //如果第一行是<VIM>或者<vim>，则打开代码阅读
	// 空格为&nbsp; 换行为 <br />
	// 每行第一个字符为#，则将这一行的<>转化成符号

	int all_len = strlen(str);
	char * pos = str;
	char * findPos = NULL;

	// 判断是不是HTML
	if ((findPos = strstr(str, "<HTML>")) == NULL || findPos > 5)
		findPos = strstr(str, "<html>");
	if (findPos != NULL && findPos < str + 5) // 是HTML
	{
		cgiHeaderContentType("text/html;charset=gbk");
		printf("%s", str); // 直接输出
		return;
	}

	// 判断是不是text
	if ((findPos = strstr(str, "<TEXT>")) == NULL || findPos > 5)
		findPos = strstr(str, "<text>");
	if (findPos != NULL && findPos < str + 5) // 是text
	{
		cgiHeaderContentType("text/plain;charset=gbk");
		if (*(findPos + 6) == '\n')
			printf("%s", str + 7); // 直接输出
		else
			printf("%s", findPos + 6);
		return;
	}

	// 判断是不是code
	if ((findPos = strstr(str, "<CODE>")) == NULL || findPos > 5)
		findPos = strstr(str, "<code>");
	if (findPos != NULL && findPos < str + 5) // 是text
	{
		cgiHeaderContentType("text/html;charset=gbk");

		char * endPos = findPos + 6;
		while (*(endPos) == ' ') endPos++;
		if (*endPos == '<')
		{
			char * endPos2 = endPos + 1;
			while ((*endPos2 >= 'a' && *endPos2 <= 'z') || (*endPos2 >= 'A' && *endPos2 <= 'Z') || *endPos2 == '-' || *endPos2 == '_' || *endPos2 == ':')
				*endPos2++;
			if (*endPos2 == '>' && endPos2 - endPos < 20) // <code><lang>格式
			{
				char lang[1000];
				int i;
				for (i = 1; endPos + i < endPos2; i++)
				{
					lang[i - 1] = *(endPos + i);
				}
				lang[i - 1] = 0;

				if (*(endPos2 + 1) == '\n')
					prt_coder(endPos2 + 2, lang); // 直接输出
				else
					prt_coder(endPos2 + 1, lang);
				return;
			}
		}

		if (*(findPos + 6) == '\n')
			prt_code(str + 7); // 直接输出
		else
			prt_code(findPos + 6);
		return;
	}

	//判断有没有重定向至网站
	if ((findPos = strstr(str, "<URL>")) == NULL || findPos > str + 5)
		findPos = strstr(str, "<url>");
	if (findPos != NULL && findPos < str + 5) // 是URL
	{
		char * endPos = NULL;
		if ((endPos = strstr(findPos, "</URL>")) == NULL)
			endPos = strstr(findPos, "</url>");
		if (endPos == NULL)
			endPos = strstr(findPos, "\n");
		if (endPos == NULL)
			endPos = str + all_len;

		char url[10000] = { 0 };
		int urllen = 0;

		for (char * p = findPos + 5; p < endPos; p++)
		{
			url[urllen++] = *p;
		}

		if (strstr(url, "http") == NULL) // 没有HTTP协议时
		{
			char url1[1000] = "http://";
			strcat(url1, url);
			strcpy(url, url1);
		}

		cgiHeaderLocation(url);
		cgiHeaderContentType("text/html;charset=gbk");
		return;
	}

	cgiHeaderContentType("text/html;charset=gbk");

	//判断有没有自定义标题
	char titletext[1000] = "乱藏空间 - 云文字";
	if ((findPos = strstr(str, "<TITLE>")) == NULL || findPos > str + 5)
		findPos = strstr(str, "<title>");

	if (findPos != NULL && findPos < str + 5) // 修改TITLE
	{
		char * endPos = NULL;
		if ((endPos = strstr(findPos, "</TITLE>")) == NULL)
			endPos = strstr(findPos, "</title>");
		if (endPos == NULL) endPos = str + all_len;
		char * nextlinePos = strstr(findPos, "\n"); // 寻找换行位置
		int ifbreak = 0;

		if (nextlinePos == NULL) // 没有换行
		{
			if (endPos == NULL || endPos - findPos > 100 || str + all_len - findPos > 100) // 全部文字太长了
				ifbreak = 1;
		}
		else if (nextlinePos < endPos) // 末尾超过换行
		{
			endPos = nextlinePos;
		}
		else if (endPos - findPos > 100) // 标题太长了
		{
			ifbreak = 1;
		}

		if (!ifbreak)
		{
			int titlelen = 0;

			for (char * p = findPos + 7; p < endPos; p++)
			{
				titletext[titlelen++] = *p;
			}
			titletext[titlelen] = 0;
		}
		if (*endPos == '\n')
			pos = endPos + 1;
		else
		{
			pos = endPos + 8;
			if (*pos == '\n')
				pos++;
		}
	}

	//正文啦
	printf("%s%s%s", "<html><head><title>", titletext, "</title></head><body>");

	int brackets = 0; // 是否在<>内
	int parentheses = 0; // 是否在()内
	int istext = 0; // #开头的普通文本
	if (*pos == '#')
		istext = 1;
	for (; pos < str + all_len; pos++)
	{
		if (*pos == ' ')
		{
			if (!brackets) // 不在<>里面
				printf("&nbsp;");
			else
				printf(" ");
		}
		else if (*pos == '\n')
		{
			if (*(pos + 1) == '#')
				istext = 1;
			else
				istext = 0;

			if (istext || !brackets)
				printf("<br />");
			else
				printf("\n");
			brackets = parentheses = 0;
			//printf("===%d===%d===\n", brackets, parentheses);
		}
		else if (*pos == '\t')
		{
			if (!brackets) // 不在<>里面
				printf("&nbsp;&nbsp;&nbsp;&nbsp;");
			else
				printf("\t");
		}
		else if (*pos == '(')
		{
			parentheses++;
			printf("(");
		}
		else if (*pos == ')')
		{
			if (parentheses > 0)
				parentheses--;
			printf(")");
		}
		else if (*pos == '<')
		{
			if (istext || parentheses) // 括号内不算
				printf("&lt;");
			else
			{
				if (*(pos + 1) == '<')
				{
					printf("&lt;&lt;");
					pos++;
				}
				else
				{
					printf("<");
					brackets++;
				}
			}
		}
		else if (*pos == '>')
		{
			if (*(pos + 1) == '>')
			{
				printf("glt;glt;");
				brackets++;
				pos++;
			}
			else if (istext || brackets <= 0 || parentheses)
				printf("&gt;");
			else
				printf(">");
			if (brackets > 0)
				brackets--;
		}
		else
		{
			printf("%c", *pos);
		}
	}
	printf("</body></html>");
}
void prt_list(char * str)
{
	if (strlen(str) <= 2)
	{
		if (*str == '_')
			prt_enter("名称以‘_’符号结尾可查找所有相关空间");
		else
			prt_enter("");
		return;
	}

	printf("<html><head><title>乱藏空间 - 云文字</title></head><body>");
	printf("以下为【%s】的搜索结果<br /><br />", str);
	char sql[1024];
	char ss[1024];
	int acount = 0;
	int len = strlen(str);
	str[len++] = '%';
	str[len] = 0;
	mysql_hex_string(ss, str, strlen(str));
	sprintf(sql, "select name,info,code from misplace where name like 0x%s", ss);
	//sprintf(sql, "select name,info,code from misplace where name like 'acm_%%'", str); // 不管sql注入的方式
	MYSQL_RES * res = executeQuery(sql);
	MYSQL_ROW row;
	printf("<ol>");
	while (row = mysql_fetch_row(res))
	{
		acount++;
		char * name = row[0];
		char * info = row[1];
		char * code = row[2];
		printf("<li><a href='misplace.cgi?name=%s' target='_blank'>%s</a>　字数：%d　<a href='misplace.cgi?read=%s' target='_blank'>分享</a></li>", name, name, strlen(info), code);
	}
	printf("</ol>");
	if (!acount)
		printf("没有找到以此开头的空间名");
	else
		printf("\n共找到：%d 项\n", acount);
	printf("</body></html>");
}

void addEditTimes(char * name);
void addSaveTimes(char * name);
void addReadTimes(char * scode);
int findReadTimes(char * sname);

void returnshare(char * sharecode)
{
	char sql[1024] = { 0 };
	char words[102400] = { 0 }, pwords[102400] = { 0 };
	int len = strlen(sharecode);
	for (int i = 0; i < len; i++)
		if ((sharecode[i] >= '0' && sharecode[i] <= '9') || (sharecode[i] >= 'a' && sharecode[i] <= 'z') || (sharecode[i] >= 'A' && sharecode[i] <= 'Z')) // 正确的分享码格式
			;
		else // 错误的分享码格式
		{
			cgiHeaderContentType("text/html;charset=gbk");
			printf("分享码错误：%s", sharecode[i]);
			return;
		}

	setNowTime();
	sprintf(sql, "update misplace set time2='%s',UA2='%s',IP2='%s' where code='%s'", nowtime, cgiUserAgent, cgiRemoteAddr, sharecode);
	executeNonQuery(sql);

	sprintf(sql, "select info from misplace where code='%s'", sharecode);
	MYSQL_RES * res = executeQuery(sql);
	MYSQL_ROW row;
	if (!(row = mysql_fetch_row(res))) // 没有
	{
		cgiHeaderContentType("text/plain;charset=gbk");
		printf("不存在这段分享的文字\n\n\n您的信息：\n");
		printf("UserAgent:%s\nAddr:%s\nHost:%s\nIdent:%s\nUser:%s\n", cgiUserAgent, cgiRemoteAddr, cgiRemoteHost, cgiRemoteIdent, cgiRemoteUser);
	}
	else // 找到只读的文本
	{
		addReadTimes(sharecode);

		prt_read(row[0]);
	}
}

int cgiMain()
{
	char name[10000] = { 0 };

	if (cgiFormString("read", sharecode, sizeof(sharecode)) == cgiFormSuccess || cgiFormString("r", sharecode, sizeof(sharecode)) == cgiFormSuccess) // 只读
	{
		returnshare(sharecode);
		return 0;
	}
	else if (cgiFormSubmitClicked("sub_share") == cgiFormSuccess) // 分享 + 判断保存
	{
		if (cgiFormString("name", name, sizeof(name)) == cgiFormSuccess) // 分享按钮
		{
			nameAndFlushCode(name);
			if (strlen(name) > 0)
			{
				char info[100000] = { 0 };
				cgiFormString("info", info, sizeof(info));
				if (strlen(info))
					saveinfo(name, info);
				else
				{
					cgiHeaderContentType("text/html;charset=gbk");
					prt_enter("信息不能为空");
				}
			}

			getShareCode(name);
			char url[1024] = { 0 };
			sprintf(url, "misplace.cgi?read=%s", sharecode);
			cgiHeaderLocation(url);
			cgiHeaderContentType("text/html;charset=gbk");
		}
		else
		{
			cgiHeaderContentType("text/html;charset=gbk");
			prt_enter("名字不能为空");
		}

		return 0;
	}

	cgiHeaderContentType("text/html;charset=gbk"); // 确定是HTML

	if (cgiFormSubmitClicked("sub_vim") == cgiFormSuccess || cgiFormSubmitClicked("vim") == cgiFormSuccess)
		gotovim = 1;
	if ((cgiFormSubmitClicked("sub_enter") == cgiFormSuccess || cgiFormSubmitClicked("sub_keep") != cgiFormSuccess) &&
		(cgiFormString("name", name, sizeof(name)) == cgiFormSuccess || cgiFormString("n", name, sizeof(name)) == cgiFormSuccess)) // 仅前往，如果没有则新建一个
	{
		//if ((cgiFormString("name", name, sizeof(name)) != cgiFormSuccess && cgiFormString("n", name, sizeof(name)) != cgiFormSuccess) || strlen(name) == 0)
		
		printf("hhh11");
		if (strlen(name) == 0)
		{
			prt_enter("进入的空间名称不能为空");
			return -1;
		}
		else if (*(name + strlen(name) - 1) == '_')
		{
			prt_list(name);
			return 0;
		}

		char sql[102400] = { 0 };
		char n0[10000] = { 0 }, i0[100000] = { 0 };

		if (!nameAndFlushCode(name)) buildRandShareCode(); // 如果没有指定sharecode，则创建
		setNowTime();

		mysql_hex_string(n0, name, strlen(name));
		sprintf(sql, "select * from misplace where name=0x%s", n0);
		MYSQL_RES * res = executeQuery(sql);
		MYSQL_ROW row;
		if (!(row = mysql_fetch_row(res))) // 没有，则新建
		{
			sprintf(sql, "insert into misplace(name,code,time,UA,IP) value(0x%s,'%s','%s','%s','%s')", n0, sharecode, nowtime, cgiUserAgent, cgiRemoteAddr);
			executeNonQuery(sql);
			prt_editor(name, "");
		}
		else // 找到，返回信息
		{
			prt_editor(name, row[2]);
			findReadTimes(name);
			printf("hhh44");
			return 0;
		}
		addEditTimes(name);
	}
	else if (cgiFormSubmitClicked("sub_keep") == cgiFormSuccess) // 保存
	{
		char info[100000] = { 0 };
		cgiFormString("info", info, sizeof(info));
		if (cgiFormString("name", name, sizeof(name)) != cgiFormSuccess)
		{
			prt_editor_msg(name, info, "保存的空间名字不能为空");
			return -1;
		}
		nameAndFlushCode(name);
		if (strlen(info)) saveinfo(name, info);
		findReadTimes(name);
		prt_editor(name, info);
	}
	else if (cgiFormString("read", sharecode, sizeof(sharecode)) == cgiFormSuccess) // 只读
	{
		returnshare(sharecode);
		return 0;
	}
	else // 到起始页
	{
		varlist1 = TMPL_add_var(varlist1, "msg", "", 0);
		TMPL_write("misenter.htm", 0, 0, varlist1, cgiOut, cgiOut);
	}
	return 0;
}

void saveinfo(char * name, char * info) // 这是16 进制的。顺便把!<> 改成=
{
	char pro[102400] = { 0 };
	char sql[102400] = { 0 };
	char n0[10000] = { 0 }, i0[100000] = { 0 };
	int oldlen = strlen(info)-2, newlen = 0;
	for (int i = 0; i < oldlen; i++)
	{
		if (info[i] == '!' && info[i + 1] == '<' && info[i + 2] == '>')
		{
			pro[newlen++] = '=';
			i += 2;
		}
		else
			pro[newlen++] = info[i];
	}
	if (*(name + strlen(name) - 1) == '_') *(name + strlen(name) - 1) = 0;
	setNowTime();
	mysql_hex_string(n0, name, strlen(name));
	mysql_hex_string(i0, pro, strlen(pro));
	sprintf(sql, "select * from misplace where name=0x%s", n0);
	MYSQL_RES * res = executeQuery(sql);
	MYSQL_ROW row;
	if (!(row = mysql_fetch_row(res))) // 没有，则新建
	{
		if (*sharecode == 0) buildRandShareCode();
		sprintf(sql, "insert into misplace(name,info,code,time,UA,IP) value(0x%s,0x%s,'%s','%s','%s','%s')", n0, i0, sharecode, nowtime, cgiUserAgent, cgiRemoteAddr);
		executeNonQuery(sql);
	}
	else // 找到，更新数据
	{
		sprintf(sql, "update misplace set name=0x%s,info=0x%s,time1='%s',UA1='%s',IP1='%s' where name=0x%s", n0, i0, nowtime, cgiUserAgent, cgiRemoteAddr, n0);
		executeNonQuery(sql);
	}
	addSaveTimes(name);
}

void executeNonQuery(char * sql)
{
	MYSQL * pConn = mysql_init(0);
	if (!mysql_real_connect(pConn, "localhost", MySQL_username, MySQL_password, "useful", 0, 0, 0)) {
		printf("连接失败：%s\n", mysql_error(pConn));
		return "";
	}
	if (mysql_query(pConn, "set names gbk")) {
		printf("设置GBL失败：%s\n", mysql_error(pConn));
		return "";
	}

	if (mysql_query(pConn, sql)) {
		printf("查询失败：%s\n", mysql_error(pConn));
		return "";
	}
	mysql_close(pConn);
}

MYSQL_RES * executeQuery(char * sql)
{
	MYSQL * pConn = mysql_init(0);
	if (!mysql_real_connect(pConn, "localhost", MySQL_username, MySQL_password, "useful", 0, 0, 0)) {
		printf("连接失败：%s\n", mysql_error(pConn));
		return "";
	}
	if (mysql_query(pConn, "set names gbk")) {
		printf("设置GBL失败：%s\n", mysql_error(pConn));
		return "";
	}

	if (mysql_query(pConn, sql)) {
		printf("查询失败：%s\n", mysql_error(pConn));
		return "";
	}

	MYSQL_RES * res = mysql_store_result(pConn);
	mysql_close(pConn);
	return res;
}

void buildRandShareCode() // 生成六位数随机码
{
	int x;
	srand((int)time(0));
	for (int i = 0; i < 6; i++)
	{
		x = rand() % 62;
		if (x < 10)
			sharecode[i] = x + '0';
		else if (x < 36)
			sharecode[i] = x + 'a' - 10;
		else
			sharecode[i] = x + 'A' - 36;
	}
	sharecode[6] = 0;

	char sql[1024] = { 0 };
	sprintf(sql, "select code from misplace where code='%s'", sharecode);
	MYSQL_RES * res = executeQuery(sql);
	MYSQL_ROW row;
	if ((row = mysql_fetch_row(res))) // 已经存在，重造
		buildRandShareCode();
}

void getShareCode(char * name) // 根据 name 获取 code
{
	char sql[1024] = { 0 };
	sprintf(sql, "select code from misplace where name='%s'", name);
	MYSQL_RES * res = executeQuery(sql);
	MYSQL_ROW row;
	if ((row = mysql_fetch_row(res))) // 找到 code
	{
		strcpy(sharecode, row[0]);
	}
	else
	{
		sharecode[0] = '0';
	}
}

void setNowTime() // 设置当前的时间
{
	time_t rawtime;
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	sprintf(nowtime, "%s", asctime(timeinfo));
}

void addEditTimes(char * sname) // 进入的次数。区分 IP1
{
	char lastIP[100] = { 0 };
	char sql[1024] = { 0 };
	int lasttimes = 0;

	{
		sprintf(sql, "select IP1 from misplace where name='%s'", sname);
		MYSQL_RES * res = executeQuery(sql);
		MYSQL_ROW row;
		if ((row = mysql_fetch_row(res)) && strlen(row)) // 存在 IP
		{
			strcpy(lastIP, row[0]);
		}
	}

	if (strcmp(lastIP, cgiRemoteAddr) != 0)
	{
		sprintf(sql, "select edittimes from misplace where name='%s'", sname);
		MYSQL_RES * res2 = executeQuery(sql);
		MYSQL_ROW row2;
		if ((row2 = mysql_fetch_row(res2))) // 存在 次数
		{
			sscanf(row2[0], "%d", &lasttimes);
		}
		else
			lasttimes = 0;

		sprintf(sql, "update misplace set edittimes=%d where name='%s'", ++lasttimes, sname);
		executeNonQuery(sql);
	}
}

void addSaveTimes(char * sname) // 保存的次数
{
	char sql[1024] = { 0 };
	int lasttimes = 0;

	sprintf(sql, "select savetimes from misplace where name='%s'", sname);
	MYSQL_RES * res = executeQuery(sql);
	MYSQL_ROW row;
	if ((row = mysql_fetch_row(res))) // 存在 次数
	{
		sscanf(row[0], "%d", &lasttimes);
	}
	else
		lasttimes = 0;

	sprintf(sql, "update misplace set savetimes=%d where name='%s'", ++lasttimes, sname);
	executeNonQuery(sql);
}

void addReadTimes2(char * scode) // 阅读的次数。区分 IP2
{
	char sql[1024] = { 0 };
	int lasttimes = 0;

	sprintf(sql, "select loadtimes from misplace where code='%s'", scode);
	MYSQL_RES * res = executeQuery(sql);
	MYSQL_ROW row;
	if ((row = mysql_fetch_row(res))) // 存在 次数
	{
		sscanf(row[0], "%d", &lasttimes);
	}
	else
		lasttimes = 0;

	sprintf(sql, "update misplace set loadtimes=%d where code='%s'", ++lasttimes, scode);
	executeNonQuery(sql);
}

void addReadTimes(char * scode) // 阅读的次数。区分 IP2
{
	char lastIP[100] = { 0 };
	char sql[1024] = { 0 };
	int lasttimes = 0;

	{
		sprintf(sql, "select IP2 from misplace where code='%s'", scode);
		MYSQL_RES * res = executeQuery(sql);
		MYSQL_ROW row;
		if ((row = mysql_fetch_row(res)) && strlen(row)) // 存在 IP
		{
			strcpy(lastIP, row[0]);
		}
	}

	if (strcmp(lastIP, cgiRemoteAddr) != 0)
	{
		sprintf(sql, "select readtimes from misplace where code='%s'", scode);
		MYSQL_RES * res = executeQuery(sql);
		MYSQL_ROW row;
		if ((row = mysql_fetch_row(res))) // 存在 次数
		{
			sscanf(row[0], "%d", &lasttimes);
		}
		else
			lasttimes = 0;

		sprintf(sql, "update misplace set readtimes=%d where code='%s'", ++lasttimes, scode);
		executeNonQuery(sql);
	}

	addReadTimes2(scode);
}

int findReadTimes(char * sname) // 获取并修改阅读次数
{
	return 0;

	char sql[1024] = { 0 };
	int readtimes = 0;

	sprintf(sql, "select readtimes from misplace where name='%s'", sname);
	MYSQL_RES * res = executeQuery(sql);
	MYSQL_ROW row;
	if ((row = mysql_fetch_row(res))) // 存在 次数
	{
		sscanf(row[0], "%d", &readtimes);
	}
	else
		readtimes = 0;

	char readstr[100] = { 0 };
	sprintf(readstr, "%d 阅读", readtimes);
	if (readtimes)
		varlist1 = TMPL_add_var(varlist1, "read", readstr, 0);

	return readtimes;
}

int nameAndCode(char * name) // name::code 格式，若处理返回1，否则返回0
{
	char * chch = NULL;
	chch = strstr(name, "::");
	if (!chch) return 0;

	// 去掉末尾的:和中文
	int len = strlen(name);
	while (len && (*(name + len - 1) == ':' || (char)(*(name + len - 1)) < 0))
		*(name + (--len)) = 0;
	if (!len) return 0;

	len = strlen(name);
	chch = strstr(name, "::");
	if (!chch) return 0;

	// 开始调整 name 和 code
	int sc = 0;
	*(chch) = *(chch + 1) = 0;
	chch += 2;
	while (*chch != 0)
	{
		sharecode[sc++] = *chch;
		chch++;
	}
	sharecode[sc] = 0;

	return 1;
}

int nameAndFlushCode(char * name)
{
	if (nameAndCode(name))
	{
		char sql[1024] = { 0 };
		sprintf(sql, "select code from misplace where code='%s'", sharecode);
		MYSQL_RES * res = executeQuery(sql);
		MYSQL_ROW row;
		if (row = mysql_fetch_row(res)) // 已经存在相同的sharecode
		{
			if (strstr(name, "mrxy_") != NULL)
			{
				sprintf(sql, "update misplace set code='' where code='%s'", sharecode); // 使用管理者权限清空其他sharecode
				executeNonQuery(sql);
			}
			else
			{
				*sharecode = 0;
				return 0;
			}
		}

		sprintf(sql, "update misplace set code='%s' where name='%s'", sharecode, name);
		executeNonQuery(sql);
		return 1;
	}
	return 0;
}