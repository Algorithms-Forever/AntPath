#pragma warning( disable : 4996)
#include <iostream>
#include <string>
#include <cmath>
#include <ctime>
#include <mysql.h>

constexpr int MaxPlace = 120;//��������
constexpr int Q = 10;//��Ϣ����صĳ���ֵ
constexpr int RoundAntNum = 10;//ÿ��ʹ��������
constexpr int CarCapacity = 100;//������

using namespace std;

struct City//����
{
	string name;//����
	int x_place=0, y_place=0;//����
	int number=0;//���
	int demand = 0;//����
};

struct Graph //��ͼ �ڽӾ���洢
{
	double** adjacentMatrix = new double* [MaxPlace],      //�ڽӾ���
		** inspiringMatrix = new double* [MaxPlace],             //�������Ӿ���
		** pheromoneMatrix = new double* [MaxPlace];        //��Ϣ�ؾ���

	City *ver = new City[MaxPlace];//���㼯
	int num_ver = 0;//�������
};

struct Ant
{
	int cityNumber = 0;//��ǰ�������б��
	int* passPath = new int[2 * MaxPlace];//·���켣���澭���ĳ��б�ţ�������������ȫ�̸���Ϣ��ȷ��
	int surplusCapacity = CarCapacity;//ʣ������
	
	double sumLength=0;//�߹�����·������
	int count = 0;//��¼���߹��ĳ�����
};

//��������
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

//�ͷ�����
void FreeConnect(MYSQL *mysql)//�ͷ�����
{
	mysql_close(mysql);
}

//������
void CreateTables (Graph*& G, MYSQL *mysql,string min, int paths)
{
	string ifExist = "DROP TABLE IF EXISTS `";

	//����place ��
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

	//�����ڽӾ����������ӣ���Ϣ�ر�
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

	//��������·����antpath, ÿ��������·����path
	string antPathTableName = "antPath" + min;
	string ifExistAntPath = ifExist + antPathTableName + "`;";

	string pathTableName = "path" + min;
	string ifExistPath = ifExist + pathTableName + "`;";

	string createStatementAntPath = "CREATE TABLE `" + antPathTableName + "`  ( `times` int(0) NOT NULL COMMENT '��¼����',\
		 `number` int(0) NOT NULL COMMENT '��¼���ϱ��',";
	string createStatementPath = "CREATE TABLE `" + pathTableName + "`  (  `times` int(0) NOT NULL COMMENT '��¼����',";
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

//��������������
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

//������������Ϣ
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

//�����һ�� Matrix�Ǿ��� length�Ǿ����С scope�Ǳ���
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

//����������Ϣ������ͼ
void CreateGraph(Graph*& G, int num_city, string* cityName, double** Matrix,int *demand, int* x_place, int* y_place)
{
	for (int i = 0; i < num_city; i++)//��ͼ�ĸ�������ϱ�ź͵�ַ,����
	{
		G->ver[i].name = cityName[i];
		G->ver[i].number = i;
		G->ver[i].x_place = x_place[i];
		G->ver[i].y_place = y_place[i];
		G->ver[i].demand = demand[i];
	}

	//ͼ�ĳ�ʼ��
	for (int i = 0; i < num_city; i++)
	{
		G->adjacentMatrix[i] = new double[num_city];
		G->inspiringMatrix[i] = new double[num_city];
		G->pheromoneMatrix[i] = new double[num_city];
	}

	//ͼ�ĸ�����ֵ
	for (int i = 0; i < num_city; i++)
		for (int j = i ; j < num_city; j++)
		{
			G->adjacentMatrix[j][i] = G->adjacentMatrix[i][j] = int(Matrix[i][j]);//�ڽӾ��� ��Χ[0-100]
			G->inspiringMatrix[j][i] = G->inspiringMatrix[i][j] = (Matrix[i][j] != 0 ? 1 / Matrix[i][j] : 0);//�������Ӿ���
			G->pheromoneMatrix[j][i] = G->pheromoneMatrix[i][j] = (i == j ? 0 : 1);//��Ϣ�ؾ���
		}
	G->num_ver = num_city;
}

//�ж�Ԫ��a�Ƿ��ڼ���array��
bool IsInSet(int a, int* array,int num)
{
	for (int i = 0; i < num; i++)
		if (array[i] == a) return true;
	return false;
}

//�����ƶ������������������㷨
void AntMove(Graph*& G,  Ant*& ant)
{
	//��ѡ����ʵ�·������״̬ת�ƹ���,�ݶ� a=1��b=1
	double* probability = new double[G->num_ver], sum = 0;
	int a = 1,b = 1;
	for (int i = 0; i < G->num_ver ; i++)
	{
		//�п�����һ����ȥ�ĵ㣬�Լ������Դ�С
		if (!IsInSet(i, ant->passPath, ant->count) && G->ver[i].demand <= ant->surplusCapacity)
			probability[i] = pow(G->pheromoneMatrix[ant->cityNumber][i], a) *
				pow(G->inspiringMatrix[ant->cityNumber][i], b);
		else probability[i] = 0;
		sum += probability[i];
	}
	
	if (sum == 0)//��û��һ�������Ҫ��
	{
		//���һ�����еĵ㶼�Ѿ��߹��ˣ�
		//���ʱ��Ӧ�������������ﷵ�أ��������ﲻ���������������

		//����������е�û�߹������ǵ�ǰ����ʣ������������
		ant->count++;//�߹���������һ
		ant->sumLength += G->adjacentMatrix[ant->cityNumber][0];//���㵱ǰ�ܳ���
		ant->passPath[ant->count-1] = 0;//д�뾭��·��
		ant->cityNumber = 0;//�޸ĵ�ǰλ��
		ant->surplusCapacity = 100;//�޸ĵ�ǰ����ʣ������
		AntMove(G, ant);//��Ϊ��ǰ�ⲽ�����ˣ����ԣ��ⲽ���㣬����һ��
		return;
	}

	//�������ֵ 0 0.1 0.2 0.3 0.05
	double RandomNum = (rand() % 100 / double(100)) * sum;
	int choose = -1;//ѡ�нڵ�ı��

	for (choose; choose < G->num_ver && RandomNum>=0; )//ȷ��ѡ�еı��
		RandomNum -= probability[++choose];

	//ȷ����һ�����ĺ󣬶����ϵ����Խ����޸ģ��Լ�¼·��
	ant->count++;//�߹���������һ
	ant->sumLength += G->adjacentMatrix[ant->cityNumber][choose];//���㵱ǰ�ܳ���
	ant->passPath[ant->count-1] = choose;//д�뾭��·��
	ant->cityNumber = choose;//�޸ĵ�ǰλ��
	ant->surplusCapacity -= G->ver[choose].demand;//�޸�����ʣ������
	return;
}

//��������·��
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

//���뵽�����߹���·����  �ѷ���
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

//��������·���洢�����ݿ�
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

//������Ϣ�ؾ��� �����µĲ���������������
void updateMatrix(Graph*& G, Ant* ant[],Ant* bestAnt)
{
	double** Matrix;//����Ϣ�ؾ���
	Matrix = new double* [G->num_ver];
	for (int i = 0; i < G->num_ver; i++)//��ʼ��
	{
		Matrix[i] = new double[G->num_ver];
		for (int j = 0; j < G->num_ver; j++)
			Matrix[i][j] = 0;
	}

	for (int k = 0; k < RoundAntNum; k++)
	{
		double increment = bestAnt->sumLength / (ant[k]->sumLength * 10);
		for (int i = 0; i < ant[k]->count - 2; i++)//�������
		{
			Matrix[ant[k]->passPath[i]][ant[k]->passPath[i + 1]] += increment;
		}
		Matrix[ant[k]->passPath[ant[k]->count-2]][ant[k]->passPath[0]] += increment;
	}

	for (int i = 0; i < G->num_ver; i++)
		for (int j = 0; j < G->num_ver; j++) 
		{
			G->pheromoneMatrix[i][j] *= 0.7;//��������Ϊ0.3 �������ִ��ʱЧ��
			G->pheromoneMatrix[i][j] += Matrix[i][j];//x(i,j) = ��x(i,j)+��x(i,j)	
		}
	MatrixNormalization(G->pheromoneMatrix, G->num_ver, 1);//�����һ��
	return;
}

//�������ݿ�����Ϣ�ؾ���
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
	
	int num_city;//�ܳ�����
	string cityId[MaxPlace];//������ID
	int cityDemand[MaxPlace] = { 0 };//����������
	int x_place[MaxPlace] = { 0 } , y_place[MaxPlace] = { 0 };//����������
	int sumDemand = 0, paths = 0;

	cout << "�������ܶ�������"; cin >> num_city;
	cout << "���������������id, x���꣬y���꣬����" << endl;
	for (int i = 0; i < num_city; i++)
	{
		cin >> cityId[i] >> x_place[i] >> y_place[i] >> cityDemand[i];
		sumDemand += cityDemand[i];
	}
	paths = sumDemand / CarCapacity + 1;

	double** Matrix;//�ڽӾ���
	Matrix = new double* [num_city];
	for (int i = 0; i < num_city; i++)
		Matrix[i] = new double[num_city];

	for (int i = 0; i < num_city; i++)
		for (int j = i; j < num_city; j++)
			if (i == j)  
				Matrix[j][i] = Matrix[i][j] = 0;
			else 
				Matrix[i][j] = Matrix[j][i] = sqrt(pow(x_place[i] - x_place[j], 2) + pow(y_place[i] - y_place[j], 2));
	
	MatrixNormalization(Matrix, num_city, 100);//�����һ��
	srand(time(0));

	Graph* MapCity = new Graph();
	CreateGraph(MapCity, num_city, cityId, Matrix, cityDemand, x_place, y_place);

	//���ݿ�
	MYSQL mysql;
	mysql_init(&mysql);
	if (!ConnectSQL(&mysql)) return -1;

	//��ǰʱ��
	struct tm* newtime;
	time_t long_time;
	time(&long_time); //Get time as long integer
	newtime = localtime(&long_time);
	string min = to_string(newtime->tm_min);//�õ���ǰʱ��ķ���
	
	//�������ݿ��еı��
	CreateTables(MapCity, &mysql, min, paths);
	//�����������
	InsertTablePlace(MapCity, &mysql, min);
	//���������Ϣ
	InsertTableMatrix(MapCity, &mysql, min);

	int num = 1000;//,������100��
	startTime = clock();//��ʱ��ʼ
	cout << "��ʱ��ʼ" << endl;

	string* BastPath = NULL;
	Ant* bestAntUntilNow = new Ant();
	bestAntUntilNow->sumLength = 1000000;

	while (num--)
	{
		Ant* ant[RoundAntNum];//��������100*10
		Ant* bestAnt = new Ant();
		int bestSum = 10000;
		for (int i = 0; i < RoundAntNum; i++)
		{
			ant[i] = new Ant();
			ant[i]->passPath[ant[i]->count] = 0;//��ʼ�������Ľڵ㣬��0��λ��
			ant[i]->count = 1;
			int count = 0;

			while (count++ != num_city - 1)
			{
				AntMove(MapCity, ant[i]);//�����ƶ�
			}
			ant[i]->passPath[ant[i]->count++] = 0;
			ant[i]->sumLength += MapCity->adjacentMatrix[ant[i]->cityNumber][0];

			//string *path = AntPath(ant[i], paths);//����·��
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
	endTime = clock();//��ʱ����
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