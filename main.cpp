#pragma warning( disable : 4996)
#include <iostream>
#include <string>
#include <cmath>
#include <ctime>
#include <mysql.h>

constexpr int MaxPlace = 120;//最大城市数
constexpr int Q = 10;//信息素相关的常数值
constexpr int RoundAntNum = 10;//每轮使用蚂蚁数
constexpr int CarCapacity = 100;//车容量

using namespace std;

struct City//顶点
{
	string name;//名称
	int x_place=0, y_place=0;//坐标
	int number=0;//编号
	int demand = 0;//需求
};

struct Graph //地图 邻接矩阵存储
{
	double** adjacentMatrix = new double* [MaxPlace],      //邻接矩阵
		** inspiringMatrix = new double* [MaxPlace],             //启发因子矩阵
		** pheromoneMatrix = new double* [MaxPlace];        //信息素矩阵

	City *ver = new City[MaxPlace];//顶点集
	int num_ver = 0;//顶点个数
};

struct Ant
{
	int cityNumber = 0;//当前所处城市编号
	int* passPath = new int[2 * MaxPlace];//路径轨迹，存经过的城市编号，用来后面走完全程给信息素确认
	int surplusCapacity = CarCapacity;//剩余容量
	
	double sumLength=0;//走过的总路径长度
	int count = 0;//记录已走过的城市数
};

//建立连接
bool ConnectSQL(MYSQL *mysql)
{
	const char* host = "127.0.0.1";
	const char* user = "root";
	const char* pass = "123456";
	const char* db = "ACO";
	if (!mysql_real_connect(mysql, host, user, pass, db, 3306, 0, 0))
		return false ;
	return  true;
}

//释放连接
void FreeConnect(MYSQL *mysql)//释放连接
{
	mysql_close(mysql);
}

//创建表
void CreateTables (Graph*& G, MYSQL *mysql,string min, int paths)
{
	string ifExist = "DROP TABLE IF EXISTS `";

	//创建place 表
	string placeTableName = "place" + min;
	string ifExistPlace = ifExist + placeTableName + "`;";
	string createStatementPlace = "CREATE TABLE `" + placeTableName + "`  (\
		`id_num` int(0) NOT NULL,\
		`x_place` int(0) NOT NULL,\
		`y_place` int(0) NOT NULL,\
		`demand` int(0) NOT NULL,\
		PRIMARY KEY(`id_num`) USING BTREE\
		) ENGINE = InnoDB CHARACTER SET = utf8 COLLATE = utf8_general_ci ROW_FORMAT = Dynamic; ";

	mysql_query(mysql, ifExistPlace.c_str());
	if (mysql_query(mysql, createStatementPlace.c_str())) cout << "Create Table " + placeTableName + " defeated!" << endl;

	//创建邻接矩阵，启发因子，信息素表
	string* vers = new string[G->num_ver];
	for (int i = 0; i < G->num_ver; i++)
		vers[i] = "toVertex" + to_string(i);

	string adjacentMatrixTableName = "adjacentMatrix" + min;
	string inspiringMatrixTableName = "inspiringMatrix" + min;
	string pheromoneMatrixTableName = "pheromoneMatrix" + min;

	string ifExistAdjacent = ifExist + adjacentMatrixTableName + "`;";
	string ifExistInspiring = ifExist + inspiringMatrixTableName + "`;";
	string ifExistpheromone = ifExist + pheromoneMatrixTableName + "`;";

	string createStatementAdjacent = "CREATE TABLE `" + adjacentMatrixTableName + "`  (  `ver_num` int(0) NOT NULL ,";
	string createStatementInspiring = "CREATE TABLE `" + inspiringMatrixTableName + "`  (  `ver_num` int(0) NOT NULL ,";
	string createStatementPheromone = "CREATE TABLE `" + pheromoneMatrixTableName + "`  (  `ver_num` int(0) NOT NULL ,";
	for (int i = 0; i < G->num_ver; i++)
	{
		createStatementAdjacent += "`" + vers[i] + "` double(10, 2) NOT NULL,";
		createStatementInspiring += "`" + vers[i] + "` double(10, 5) NOT NULL,";
		createStatementPheromone += "`" + vers[i] + "` double(10, 5) NOT NULL,";
	}
	createStatementAdjacent += "PRIMARY KEY (`ver_num`) USING BTREE\
		) ENGINE = InnoDB CHARACTER SET = utf8 COLLATE = utf8_general_ci ROW_FORMAT = Dynamic; ";
	createStatementInspiring += "PRIMARY KEY (`ver_num`) USING BTREE\
		) ENGINE = InnoDB CHARACTER SET = utf8 COLLATE = utf8_general_ci ROW_FORMAT = Dynamic; ";
	createStatementPheromone += "PRIMARY KEY (`ver_num`) USING BTREE\
		) ENGINE = InnoDB CHARACTER SET = utf8 COLLATE = utf8_general_ci ROW_FORMAT = Dynamic; ";

	mysql_query(mysql, ifExistAdjacent.c_str());
	if(mysql_query(mysql, createStatementAdjacent.c_str())) cout << "Create Table " + adjacentMatrixTableName + " defeated!" << endl;
	mysql_query(mysql, ifExistInspiring.c_str());
	if(mysql_query(mysql, createStatementInspiring.c_str())) cout << "Create Table " + inspiringMatrixTableName + " defeated!" << endl;
	mysql_query(mysql, ifExistpheromone.c_str());
	if(mysql_query(mysql, createStatementPheromone.c_str())) cout << "Create Table " + pheromoneMatrixTableName + " defeated!" << endl;

	//创建蚂蚁路径表antpath, 每代的最优路径表path
	string antPathTableName = "antPath" + min;
	string ifExistAntPath = ifExist + antPathTableName + "`;";

	string pathTableName = "path" + min;
	string ifExistPath = ifExist + pathTableName + "`;";

	string createStatementAntPath = "CREATE TABLE `" + antPathTableName + "`  ( `times` int(0) NOT NULL COMMENT '记录代次',\
		 `number` int(0) NOT NULL COMMENT '记录蚂蚁编号',";
	string createStatementPath = "CREATE TABLE `" + pathTableName + "`  (  `times` int(0) NOT NULL COMMENT '记录代次',";
	for (int i = 0; i < paths; i++)
	{
		createStatementAntPath += "`path" + to_string(i) + "` tinytext CHARACTER SET utf8 COLLATE utf8_general_ci NULL,";
		createStatementPath += "`path" + to_string(i) + "`  tinytext CHARACTER SET utf8 COLLATE utf8_general_ci NULL,";
	}
	createStatementAntPath += "`length` int(0) NOT NULL,\
		PRIMARY KEY (`times`,`number`) USING BTREE\
		) ENGINE = InnoDB CHARACTER SET = utf8 COLLATE = utf8_general_ci ROW_FORMAT = Dynamic; ";
	createStatementPath += "`length` int(0) NOT NULL,\
		PRIMARY KEY (`times`) USING BTREE\
		) ENGINE = InnoDB CHARACTER SET = utf8 COLLATE = utf8_general_ci ROW_FORMAT = Dynamic; ";

	mysql_query(mysql, ifExistAntPath.c_str());
	if(mysql_query(mysql, createStatementAntPath.c_str())) cout << "Create Table " + antPathTableName + " defeated!" << endl;
	mysql_query(mysql, ifExistPath.c_str());
	if(mysql_query(mysql, createStatementPath.c_str())) cout << "Create Table " + pathTableName + " defeated!" << endl;
}

//插入城市相关数据
void InsertTablePlace(Graph*& G, MYSQL *mysql,string min)
{
	string TableName = "place" + min;
	string InsertStatement = "INSERT INTO `" + TableName + "` VALUES (";
	for (int i = 0; i < G->num_ver; i++)
	{
		InsertStatement = InsertStatement + to_string(G->ver[i].number) + "," + to_string(G->ver[i].x_place) + "," + to_string(G->ver[i].y_place) + "," + to_string(G->ver[i].demand) + ");";
		if (mysql_query(mysql, InsertStatement.c_str())) cout << "Insert to place table record " + to_string(i + 1) + " defeated!" << endl;
		InsertStatement = "INSERT INTO `" + TableName + "` VALUES (";
	}
}

//插入矩阵相关信息
void InsertTableMatrix(Graph*& G, MYSQL *mysql, string min)
{
	string adjacentMatrixTableName = "adjacentMatrix" + min;
	string inspiringMatrixTableName = "inspiringMatrix" + min;
	string pheromoneMatrixTableName = "pheromoneMatrix" + min;

	string InsertStatementad = "INSERT INTO `" + adjacentMatrixTableName + "` VALUES (";
	string InsertStatementin = "INSERT INTO `" + inspiringMatrixTableName + "` VALUES (";
	string InsertStatementph = "INSERT INTO `" + pheromoneMatrixTableName + "` VALUES (";

	for (int i = 0; i < G->num_ver; i++)
	{
		InsertStatementad = InsertStatementad + to_string(i) + ",";
		InsertStatementin = InsertStatementin + to_string(i) + ",";
		InsertStatementph = InsertStatementph + to_string(i) + ",";
		for (int j = 0; j < G->num_ver; j++)
		{
			if (j != G->num_ver - 1) {
				InsertStatementad = InsertStatementad + to_string(G->adjacentMatrix[i][j]) + ",";
				InsertStatementin = InsertStatementin + to_string(G->inspiringMatrix[i][j]) + ",";
				InsertStatementph = InsertStatementph + to_string(G->pheromoneMatrix[i][j]) + ",";
			}
			else {
				InsertStatementad = InsertStatementad + to_string(G->adjacentMatrix[i][j]) + ");";
				InsertStatementin = InsertStatementin + to_string(G->inspiringMatrix[i][j]) + ")";
				InsertStatementph = InsertStatementph + to_string(G->pheromoneMatrix[i][j]) + ")";
			}
		}
		if(mysql_query(mysql, InsertStatementad.c_str())) cout << "Insert to place adjacent record " + to_string(i + 1) + " defeated!" << endl;
		if(mysql_query(mysql, InsertStatementin.c_str())) cout << "Insert to place inspring record " + to_string(i + 1) + " defeated!" << endl;
		if(mysql_query(mysql, InsertStatementph.c_str())) cout << "Insert to place pheromone record " + to_string(i + 1) + " defeated!" << endl;;
		InsertStatementad = "INSERT INTO `" + adjacentMatrixTableName + "` VALUES (";
		InsertStatementin = "INSERT INTO `" + inspiringMatrixTableName + "` VALUES (";
		InsertStatementph = "INSERT INTO `" + pheromoneMatrixTableName + "` VALUES (";
	}

}

//矩阵归一化 Matrix是矩阵 length是矩阵大小 scope是倍率
void MatrixNormalization(double** Matrix, int length, int scope)
{
	double max = -1;
	for (int i = 0; i < length; i++)
		for (int j = i; j < length; j++)
			if (Matrix[i][j] > max) max = Matrix[i][j];

	for (int i = 0; i < length; i++)
		for (int j = 0; j < length; j++)
		{
			Matrix[i][j] = Matrix[i][j] * scope / max;
		}
}

//依据现有信息，创建图
void CreateGraph(Graph*& G, int num_city, string* cityName, double** Matrix,int *demand, int* x_place, int* y_place)
{
	for (int i = 0; i < num_city; i++)//给图的各顶点加上编号和地址,需求
	{
		G->ver[i].name = cityName[i];
		G->ver[i].number = i;
		G->ver[i].x_place = x_place[i];
		G->ver[i].y_place = y_place[i];
		G->ver[i].demand = demand[i];
	}

	//图的初始化
	for (int i = 0; i < num_city; i++)
	{
		G->adjacentMatrix[i] = new double[num_city];
		G->inspiringMatrix[i] = new double[num_city];
		G->pheromoneMatrix[i] = new double[num_city];
	}

	//图的各矩阵赋值
	for (int i = 0; i < num_city; i++)
		for (int j = i ; j < num_city; j++)
		{
			G->adjacentMatrix[j][i] = G->adjacentMatrix[i][j] = int(Matrix[i][j]);//邻接矩阵 范围[0-100]
			G->inspiringMatrix[j][i] = G->inspiringMatrix[i][j] = (Matrix[i][j] != 0 ? 1 / Matrix[i][j] : 0);//启发因子矩阵
			G->pheromoneMatrix[j][i] = G->pheromoneMatrix[i][j] = (i == j ? 0 : 1);//信息素矩阵
		}
	G->num_ver = num_city;
}

//判断元素a是否在集合array中
bool IsInSet(int a, int* array,int num)
{
	for (int i = 0; i < num; i++)
		if (array[i] == a) return true;
	return false;
}

//蚂蚁移动，类似于拓扑排序算法
void AntMove(Graph*& G,  Ant*& ant)
{
	//先选择合适的路径，即状态转移规则,暂定 a=1，b=1
	double* probability = new double[G->num_ver], sum = 0;
	int a = 1,b = 1;
	for (int i = 0; i < G->num_ver ; i++)
	{
		//有可能下一步过去的点，以及可能性大小
		if (!IsInSet(i, ant->passPath, ant->count) && G->ver[i].demand <= ant->surplusCapacity)
			probability[i] = pow(G->pheromoneMatrix[ant->cityNumber][i], a) *
				pow(G->inspiringMatrix[ant->cityNumber][i], b);
		else probability[i] = 0;
		sum += probability[i];
	}
	
	if (sum == 0)//即没有一个点符合要求
	{
		//情况一：所有的点都已经走过了，
		//这个时候应当是在主函数里返回，所以这里不会有这种情况出现

		//情况二：还有点没走过，但是当前蚂蚁剩余容量不够了
		ant->count++;//走过城市数加一
		ant->sumLength += G->adjacentMatrix[ant->cityNumber][0];//计算当前总长度
		ant->passPath[ant->count-1] = 0;//写入经过路径
		ant->cityNumber = 0;//修改当前位置
		ant->surplusCapacity = 100;//修改当前蚂蚁剩余容量
		AntMove(G, ant);//因为当前这步回来了，所以，这步不算，再走一步
		return;
	}

	//生成随机值 0 0.1 0.2 0.3 0.05
	double RandomNum = (rand() % 100 / double(100)) * sum;
	int choose = -1;//选中节点的编号

	for (choose; choose < G->num_ver && RandomNum>=0; )//确定选中的编号
		RandomNum -= probability[++choose];

	//确定下一步往哪后，对蚂蚁的属性进行修改，以记录路径
	ant->count++;//走过城市数加一
	ant->sumLength += G->adjacentMatrix[ant->cityNumber][choose];//计算当前总长度
	ant->passPath[ant->count-1] = choose;//写入经过路径
	ant->cityNumber = choose;//修改当前位置
	ant->surplusCapacity -= G->ver[choose].demand;//修改蚂蚁剩余容量
	return;
}

//生成蚂蚁路线
string* AntPath(Ant*& ant, int paths)
{
	string* path = new string[paths+1];
	int num = 0;
	path[num] = to_string(ant->passPath[0]) + " ";
	for (int i = 1; i < ant->count-1; i++)
	{
		if (ant->passPath[i] != 0) path[num] = path[num] + to_string(ant->passPath[i]) + " ";
		else {
			path[num] += to_string(ant->passPath[i]);
			num++;
			path[num] = to_string(ant->passPath[i]) + " ";
		}
	}
	path[num] += to_string(ant->passPath[0]);
	return path;
}

//插入到蚂蚁走过的路径中  已废弃
void InsertAntPath(MYSQL* mysql, Ant*& ant,int time,int num,string *path, string min,int paths)
{
	//time num path[i] length
	string antPathTableName = "antpath" + min;
	string InsertStatement = "INSERT INTO `" + antPathTableName + "` VALUES (" + to_string(time) + "," + to_string(num) + ",";
	for (int i = 0; i < paths; i++)
		InsertStatement = InsertStatement +"\""+ path[i] + "\",";
	InsertStatement = InsertStatement +to_string(ant->sumLength) + ");";
	if (mysql_query(mysql, InsertStatement.c_str())) cout << "Insert to antPath table record " + to_string(num) + " defeated!" << endl;
}

//本轮最优路径存储到数据库
void InsertPath(MYSQL* mysql, Ant*& ant, int time, string* path, string min, int paths)
{
	//time path[i] length
	string antPathTableName = "path" + min;
	string InsertStatement = "INSERT INTO `" + antPathTableName + "` VALUES (" + to_string(time) +  ",";
	for (int i = 0; i < paths; i++)
		InsertStatement = InsertStatement + "\"" + path[i] + "\",";
	InsertStatement = InsertStatement + to_string(ant->sumLength) + ");";
	if (mysql_query(mysql, InsertStatement.c_str())) cout << "Insert to Path table record " + to_string(time) + " defeated!" << endl;
}

//更新信息素矩阵 加入新的差量，并总体蒸发
void updateMatrix(Graph*& G, Ant* ant[],Ant* bestAnt)
{
	double** Matrix;//Δ信息素矩阵
	Matrix = new double* [G->num_ver];
	for (int i = 0; i < G->num_ver; i++)//初始化
	{
		Matrix[i] = new double[G->num_ver];
		for (int j = 0; j < G->num_ver; j++)
			Matrix[i][j] = 0;
	}

	for (int k = 0; k < RoundAntNum; k++)
	{
		double increment = bestAnt->sumLength / (ant[k]->sumLength * 10);
		for (int i = 0; i < ant[k]->count - 2; i++)//计算差量
		{
			Matrix[ant[k]->passPath[i]][ant[k]->passPath[i + 1]] += increment;
		}
		Matrix[ant[k]->passPath[ant[k]->count-2]][ant[k]->passPath[0]] += increment;
	}

	for (int i = 0; i < G->num_ver; i++)
		for (int j = 0; j < G->num_ver; j++) 
		{
			G->pheromoneMatrix[i][j] *= 0.7;//蒸发因子为0.3 代表程序执行时效率
			G->pheromoneMatrix[i][j] += Matrix[i][j];//x(i,j) = ρx(i,j)+Δx(i,j)	
		}
	MatrixNormalization(G->pheromoneMatrix, G->num_ver, 1);//矩阵归一化
	return;
}

//更新数据库中信息素矩阵
void updateMatrixDataBase(MYSQL* mysql, Graph*& G, string min)
{
	//UPDATE `aco`.`pheromonematrix43` SET  `toVertex0` = 1 WHERE `ver_num` = 2
	string TableName = "pheromoneMatrix" + min;
	string InsertStatement = "UPDATE `aco`.`" + TableName + "` SET  ";

	for (int i = 0; i < G->num_ver; i++)
	{
		for (int j = 0; j < G->num_ver; j++)
		{
			if (j != G->num_ver - 1) InsertStatement = InsertStatement + "`toVertex" + to_string(j) + "` = " + to_string(G->pheromoneMatrix[i][j]) + ",";
			else InsertStatement = InsertStatement + "`toVertex" + to_string(j) + "` = " \
				+ to_string(G->pheromoneMatrix[i][j]) + " WHERE `ver_num` = " + to_string(i) + ";";
		}
		if (mysql_query(mysql, InsertStatement.c_str())) cout << "Insert to antPath table record " + to_string(i) + " defeated!" << endl;
	}
}

int main()
{
	clock_t startTime, endTime;
	
	int num_city;//总城市数
	string cityId[MaxPlace];//各城市ID
	int cityDemand[MaxPlace] = { 0 };//各城市需求
	int x_place[MaxPlace] = { 0 } , y_place[MaxPlace] = { 0 };//各城市坐标
	int sumDemand = 0, paths = 0;

	cout << "请输入总顶点数："; cin >> num_city;
	cout << "请依次输入各顶点id, x坐标，y坐标，需求：" << endl;
	for (int i = 0; i < num_city; i++)
	{
		cin >> cityId[i] >> x_place[i] >> y_place[i] >> cityDemand[i];
		sumDemand += cityDemand[i];
	}
	paths = sumDemand / CarCapacity + 1;

	double** Matrix;//邻接矩阵
	Matrix = new double* [num_city];
	for (int i = 0; i < num_city; i++)
		Matrix[i] = new double[num_city];

	for (int i = 0; i < num_city; i++)
		for (int j = i; j < num_city; j++)
			if (i == j)  
				Matrix[j][i] = Matrix[i][j] = 0;
			else 
				Matrix[i][j] = Matrix[j][i] = sqrt(pow(x_place[i] - x_place[j], 2) + pow(y_place[i] - y_place[j], 2));
	
	MatrixNormalization(Matrix, num_city, 100);//矩阵归一化
	srand(time(0));

	Graph* MapCity = new Graph();
	CreateGraph(MapCity, num_city, cityId, Matrix, cityDemand, x_place, y_place);

	//数据库
	MYSQL mysql;
	mysql_init(&mysql);
	if (!ConnectSQL(&mysql)) return -1;

	//当前时间
	struct tm* newtime;
	time_t long_time;
	time(&long_time); //Get time as long integer
	newtime = localtime(&long_time);
	string min = to_string(newtime->tm_min);//得到当前时间的分钟
	
	//创建数据库中的表格
	CreateTables(MapCity, &mysql, min, paths);
	//插入城市数据
	InsertTablePlace(MapCity, &mysql, min);
	//插入矩阵信息
	InsertTableMatrix(MapCity, &mysql, min);

	int num = 1000;//,即进行100轮
	startTime = clock();//计时开始
	cout << "计时开始" << endl;

	string* BastPath = NULL;
	Ant* bestAntUntilNow = new Ant();
	bestAntUntilNow->sumLength = 1000000;

	while (num--)
	{
		Ant* ant[RoundAntNum];//总蚂蚁数100*10
		Ant* bestAnt = new Ant();
		int bestSum = 10000;
		for (int i = 0; i < RoundAntNum; i++)
		{
			ant[i] = new Ant();
			ant[i]->passPath[ant[i]->count] = 0;//初始就在中心节点，即0号位置
			ant[i]->count = 1;
			int count = 0;

			while (count++ != num_city - 1)
			{
				AntMove(MapCity, ant[i]);//蚂蚁移动
			}
			ant[i]->passPath[ant[i]->count++] = 0;
			ant[i]->sumLength += MapCity->adjacentMatrix[ant[i]->cityNumber][0];

			//string *path = AntPath(ant[i], paths);//蚂蚁路线
			//InsertAntPath(&mysql, ant[i], 100 - num, i + 1, path, min, paths);

			if (ant[i]->sumLength <= bestSum) {
				bestAnt = ant[i];
				bestSum = ant[i]->sumLength;
			}
		}

		if (bestAnt->sumLength < bestAntUntilNow->sumLength) bestAntUntilNow = bestAnt;

		if (num % RoundAntNum == 0)
		{
			updateMatrix(MapCity, ant, bestAntUntilNow);
			
			string* path = AntPath(bestAntUntilNow, paths);
			InsertPath(&mysql, bestAntUntilNow, (1000 - num)/10, path, min, paths);
		}
	}

	//updateMatrixDataBase(&mysql, MapCity, min);

	FreeConnect(&mysql);
	endTime = clock();//计时结束
	cout << "The run time is:" << (double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
	system("pause");
	return 0;
}
/*
32
1 82 76 0
2 96 44 19
3 50 5 21
4 49 8 6
5 13 7 19
6 29 89 7
7 58 30 12
8 84 39 16
9 14 24 6
10 2 39 16
11 3 82 8
12 5 10 14
13 98 52 21
14 84 25 16
15 61 59 3
16 1 65 22
17 88 51 18
18 91 2 19
19 19 32 1
20 93 3 24
21 50 93 8
22 98 14 12
23 5 42 4
24 42 9 8
25 61 62 24
26 9 97 24
27 80 55 2
28 57 69 20
29 23 15 15
30 20 70 2
31 85 60 14
32 98 5 9


80
1 92 92 0
2 88 58 24
3 70 6 22
4 57 59 23
5 0 98 5
6 61 38 11
7 65 22 23
8 91 52 26
9 59 2 9
10 3 54 23
11 95 38 9
12 80 28 14
13 66 42 16
14 79 74 12
15 99 25 2
16 20 43 2
17 40 3 6
18 50 42 20
19 97 0 26
20 21 19 12
21 36 21 15
22 100 61 13
23 11 85 26
24 69 35 17
25 69 22 7
26 29 35 12
27 14 9 4
28 50 33 4
29 89 17 20
30 57 44 10
31 60 25 9
32 48 42 2
33 17 93 9
34 21 50 1
35 77 18 2
36 2 4 2
37 63 83 12
38 68 6 14
39 41 95 23
40 48 54 21
41 98 73 13
42 26 38 13
43 69 76 23
44 40 1 3
45 65 41 6
46 14 86 23
47 32 39 11
48 14 24 2
49 96 5 7
50 82 98 13
51 23 85 10
52 63 69 3
53 87 19 6
54 56 75 13
55 15 63 2
56 10 45 14
57 7 30 7
58 31 11 21
59 36 93 7
60 50 31 22
61 49 52 13
62 39 10 22
63 76 40 18
64 83 34 22
65 33 51 6
66 0 15 2
67 52 82 11
68 52 82 5
69 46 6 9
70 3 26 9
71 46 80 5
72 94 30 12
73 26 76 2
74 75 92 12
75 57 51 19
76 34 21 6
77 28 80 14
78 59 66 2
79 51 16 2
80 87 11 24

*/