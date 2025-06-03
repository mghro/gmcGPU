// Sqlite.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "sqlite3.h"


int main()
{
  sqlite3 *db;

  int status = sqlite3_open("C:\\MyProjects\\Sqlite\\test.db", &db);


    return 0;
}

