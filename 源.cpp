#include <iostream>
#include <vector>
#include <graphics.h>
#include <stdio.h>
#include <Windows.h>
#include <fstream>

#define _CRT_SECURE_NO_WARNINGS

using namespace std;

void putimagePNG(int x, int y, IMAGE* picture) //x为载入图片的X坐标，y为Y坐标
{
	// 变量初始化
	DWORD* dst = GetImageBuffer();    // GetImageBuffer()函数，用于获取绘图设备的显存指针，EASYX自带
	DWORD* draw = GetImageBuffer();
	DWORD* src = GetImageBuffer(picture); //获取picture的显存指针
	int picture_width = picture->getwidth(); //获取picture的宽度，EASYX自带
	int picture_height = picture->getheight(); //获取picture的高度，EASYX自带
	int graphWidth = getwidth();       //获取绘图区的宽度，EASYX自带
	int graphHeight = getheight();     //获取绘图区的高度，EASYX自带
	int dstX = 0;    //在显存里像素的角标

	// 实现透明贴图 公式： Cp=αp*FP+(1-αp)*BP ， 贝叶斯定理来进行点颜色的概率计算
	for (int iy = 0; iy < picture_height; iy++)
	{
		for (int ix = 0; ix < picture_width; ix++)
		{
			int srcX = ix + iy * picture_width; //在显存里像素的角标
			int sa = ((src[srcX] & 0xff000000) >> 24); //0xAArrggbb;AA是透明度
			int sr = ((src[srcX] & 0xff0000) >> 16); //获取RGB里的R
			int sg = ((src[srcX] & 0xff00) >> 8);   //G
			int sb = src[srcX] & 0xff;              //B
			if (ix >= 0 && ix <= graphWidth && iy >= 0 && iy <= graphHeight && dstX <= graphWidth * graphHeight)
			{
				dstX = (ix + x) + (iy + y) * graphWidth; //在显存里像素的角标
				int dr = ((dst[dstX] & 0xff0000) >> 16);
				int dg = ((dst[dstX] & 0xff00) >> 8);
				int db = dst[dstX] & 0xff;
				draw[dstX] = ((sr * sa / 255 + dr * (255 - sa) / 255) << 16)  //公式： Cp=αp*FP+(1-αp)*BP  ； αp=sa/255 , FP=sr , BP=dr
					| ((sg * sa / 255 + dg * (255 - sa) / 255) << 8)         //αp=sa/255 , FP=sg , BP=dg
					| (sb * sa / 255 + db * (255 - sa) / 255);              //αp=sa/255 , FP=sb , BP=db
			}
		}
	}
}


struct ChessPos {
	int row; int col;
};
struct ChessGame {
	int mode;//1人黑棋，-1人白棋
	void play();
};
struct Chess {
	void init();//初始化棋盘
	void save();//保存棋局
	bool clickBoard(int x, int y);//判定有效点击
	void chessDown(int row, int col, int kind);//落子
	int checkWin();//检查对局是否结束
	int maxLength(int kind);//最大连子数
	bool judgeBegin();//判定当前状态是否为棋局开始
	int live4(int l, int c, int color);//某点位于几个活四中
	int cheng5(int l, int c, int color);//某点所在棋型有几个成五点
	int chong4(int l, int c, int color); //某点位于几个冲四中
	int live3(int l, int c, int color); //某点位于几个活三中
	bool overline(int l, int c);//判定长连禁手
	bool ban(int l, int c);//判定禁手
	int samenum(int l, int c, int u, int color);//某格点各方向上有多少连续同色子
	bool inboard(int l, int c);//判断
	int surround(int l, int c);//周围8格子有多少棋子


	IMAGE chessBlackImg;
	IMAGE chessWhiteImg;
	int gradeSize;//棋盘行数
	int margin_x;//棋盘左侧边界
	int margin_y;//棋盘顶部边界
	float chessSize;//棋子大小
	int chessMap[16][16];//存储当前棋盘状态，0空白，1黑子，-1白子

};
struct Man {
	void go(int kind);
};
struct AI {
	void go(int kind);
	void init();
	int calculateScore(int l, int c);
	ChessPos think();

	int scoreMap[16][16] = { 0 };
};


ChessPos pos;
Chess chess;
Man man;
AI ai;
ChessGame game;
bool markEnd = 0;
int hint = 1;
const string filename = "chessMap.txt";
int zong[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
int heng[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };

bool Chess::inboard(int l, int c) //判断某点是否在棋盘内
{
	if (l < 0 || l > 14 || c < 0 || c > 14) return false;
	else return true;
}

int Chess::samenum(int l, int c, int u, int color) //某格点各方向上有多少连续同色子
{
	for (int i = 1; ; i++) {
		if (!inboard(l + i * zong[u], c + i * heng[u]) || chess.chessMap[l + i * zong[u]][c + i * heng[u]] != color) {
			return i - 1; break;
		}
	}
}

int Chess::live4(int l, int c, int color) //某点位于几个活四中
{
	int sum = 0;
	for (int u = 0; u < 4; u++) {//遍历纵向，左上-右下，横向，左下-右上四个方向
		int p = samenum(l, c, u, color), q = samenum(l, c, u + 4, color);
		if (p + q != 3) continue;
		else {//判断两端是否有空位
			if (!inboard(l + (p + 1) * zong[u], c + (p + 1) * heng[u]) || !inboard(l + (q + 1) * zong[u + 4], c + (q + 1) * heng[u + 4])) {
				continue;
			}
			else if (chess.chessMap[l + (p + 1) * zong[u]][c + (p + 1) * heng[u]] == -color || chess.chessMap[l + (q + 1) * zong[u + 4]][c + (q + 1) * heng[u + 4]] == -color) {
								continue;
			}
			else sum++;
		}
	}
	return sum;
}

int Chess::cheng5(int l, int c, int color) //某点所在棋型有几个成五点
{
	int sum = 0;
	for (int u = 0; u < 8; u++) {
		int p = samenum(l, c, u, color);
		if (p > 3) continue;
		else if (!inboard(l + (p + 1) * zong[u], c + (p + 1) * heng[u])) {
			continue;
		}
		else if (chess.chessMap[l + (p + 1) * zong[u]][c + (p + 1) * heng[u]] == -color) {
			continue;
		}
		else {
			if (samenum(l, c, u, color) + samenum(l + (p + 1) * zong[u], c + (p + 1) * heng[u], u, color) + samenum(l, c, (u + 4) % 8, color) >= 3) {
				sum++;
			}
		}
	}
	return sum;
}

int Chess::chong4(int l, int c, int color) //某点位于几个冲四中
{
	return cheng5(l, c, color) - 2 * chess.live4(l, c, color);
}

int Chess::live3(int l, int c, int color) //某点位于几个活三中
{
	int sum1 = 0; //考虑三连情形
	for (int u = 0; u < 4; u++) {
		int p = samenum(l, c, u, color), q = samenum(l, c, u + 4, color);
		if (p + q != 2) continue;
		else {
			if (!inboard(l + (p + 1) * zong[u], c + (p + 1) * heng[u])) {
				continue;
			}
			else if (chess.chessMap[l + (p + 1) * zong[u]][c + (p + 1) * heng[u]] == -color) {
				continue;
			}
			else if (!inboard(l + (q + 1) * zong[u + 4], c + (q + 1) * heng[u + 4])) {
				continue;
			}
			else if (chess.chessMap[l + (q + 1) * zong[u + 4]][c + (q + 1) * heng[u + 4]] == -color) {
				continue;
			}
			else {
				if (inboard(l + (q + 2) * zong[u + 4], c + (q + 2) * heng[u + 4]) && chess.chessMap[l + (q + 2) * zong[u + 4]][c + (q + 2) * heng[u + 4]] == 0) {
					sum1++;
				}
				else if (inboard(l + (p + 2) * zong[u], c + (p + 2) * heng[u]) && chess.chessMap[l + (p + 2) * zong[u]][c + (p + 2) * heng[u]] == 0) {
					sum1++;
				}
			}
		}
	}
	int sum2 = 0; //非三连情形
	for (int u = 0; u < 8; u++) {
		int p1 = 0, p2 = 0, p3 = 0, p4 = 0, p5 = 0;
		if (inboard(l - zong[u], c - heng[u]) && chess.chessMap[l - zong[u]][c - heng[u]] == 0) p1 = 1;
		if (inboard(l + 4 * zong[u], c + 4 * heng[u]) && chess.chessMap[l + 4 * zong[u]][c + 4 * heng[u]] == 0) p2 = 1;
		if (chess.chessMap[l + zong[u]][c + heng[u]] + chess.chessMap[l + 2 * zong[u]][c + 2 * heng[u]] == color) p3 = 1;
		if (chess.chessMap[l + 2 * zong[u]][c + 2 * heng[u]] * chess.chessMap[l + zong[u]][c + heng[u]] == 0) p4 = 1;
		if (chess.chessMap[l + 3 * zong[u]][c + 3 * heng[u]] == color) p5 = 1;
		if (p1 * p2 * p3 * p4 * p5) sum2++;
		int q1 = 0, q2 = 0, q3 = 0, q4 = 0, q5 = 0;
		if (inboard(l - 2 * zong[u], c - 2 * heng[u]) && chess.chessMap[l - 2 * zong[u]][c - 2 * heng[u]] == 0) q1 = 1;
		if (inboard(l + 3 * zong[u], c + 3 * heng[u]) && chess.chessMap[l + 3 * zong[u]][c + 3 * heng[u]] == 0) q2 = 1;
		if (chess.chessMap[l - zong[u]][c - heng[u]] == color) q3 = 1;
		if (chess.chessMap[l + zong[u]][c + heng[u]] == 0) q4 = 1;
		if (chess.chessMap[l + 2 * zong[u]][c + 2 * heng[u]] == color) q5 = 1;
		if (q1 * q2 * q3 * q4 * q5) sum2++;
	}
	return sum1 + sum2;
}

bool Chess::overline(int l, int c) //判长连禁手
{
	for (int u = 0; u < 4; u++) {
		if (samenum(l, c, u, 1) + samenum(l, c, u + 4, 1) > 4)return true;
	}
	return false;
}

bool Chess::ban(int l, int c) //判禁手
{
	return chess.live3(l, c, 1) > 1 || chess.overline(l, c) || chess.live4(l, c, 1) + chess.chong4(l, c, 1) > 1;
}

void Chess::init()
{
	initgraph(600, 800);
	loadimage(0, "source/Board.jpg");


	//MessageBox(NULL, "这是一个警告信息", "警告", MB_OKCANCEL | MB_ICONWARNING);

	setlinecolor(BLACK);
	setbkmode(0);

	setfillcolor(RGB(185, 122, 87));
	solidrectangle(50, 666, 250, 706);
	solidrectangle(350, 666, 550, 706);
	solidrectangle(50, 726, 250, 766);
	solidrectangle(350, 726, 550, 766);

	settextcolor(BLACK);
	settextstyle(35, 0, "微软雅黑");
	outtextxy(50, 600, "玩家:");
	outtextxy(300, 600, "电脑:");
	outtextxy(50, 666, "禁手提示:开");
	outtextxy(50, 726, "认输");
	outtextxy(350, 666, "保存棋盘");
	outtextxy(350, 726, "退出游戏");//界面菜单栏

	hint = 1;
	//位置数组还原
	ifstream inFile(filename);
	for (int i = 0; i <= 14; i++) {
		for (int j = 0; j <= 14; j++) {
			inFile >> chessMap[i][j];
			if (chessMap[i][j] == 1) {
				chessDown(i, j, 1);
			}
			else if (chessMap[i][j] == -1) {
				chessDown(i, j, -1);
			}
		}
	}

	if (chess.judgeBegin()) {
		int rst = MessageBox(NULL, "请选择是否执黑：\n是: 执黑棋\n否: 执白棋", "请选择", MB_YESNO);
		
		if (rst == IDNO) {
			game.mode = -1;
			chess.chessDown(7, 7, 1);
			for (int i = 0; i <= 14; i++) {
				for (int j = 0; j <= 14; j++) {

					chessMap[i][j] = 0;
				}
			}
			chessMap[7][7] = 1;
			ofstream outFile(filename, ios::trunc);
			for (int i = 0; i <= 14; i++) {
				for (int j = 0; j <= 14; j++) {
					outFile << 0 << ' ';
				}outFile << endl;
			}
			outFile << game.mode;
		}
		else {
			game.mode = 1;
			ofstream outFile(filename, ios::trunc);
			for (int i = 0; i <= 14; i++) {
				for (int j = 0; j <= 14; j++) {
					chessMap[i][j] = 0;
					outFile << chessMap[i][j] << ' ';
				}outFile << endl;
			}
			outFile << game.mode;
		}
		
	}
	else {
		int rst = MessageBox(NULL, "是否继续上一局游戏", "请选择", MB_YESNO);
		if (rst == IDNO) {
			ofstream outFile(filename, ios::trunc);
			for (int i = 0; i <= 14; i++) {
				for (int j = 0; j <= 14; j++) {
					chessMap[i][j] = 0;
					outFile << chessMap[i][j] << ' ';
				}outFile << endl;
			}
			init();
		}
		else {
			inFile >> game.mode;
		}

	}
	inFile.close();
	if (game.mode == 1) {
		outtextxy(110, 600, "黑棋");
		outtextxy(360, 600, "白棋");
	}
	else if (game.mode == -1) {
		outtextxy(110, 600, "白棋");
		outtextxy(360, 600, "黑棋");
	}
	/*for (int i = 0; i <= 14; i++) {
		for (int j = 0; j <= 14; j++) {
			chessMap[i][j] = 0;
		}
	}*/

}

void Chess::save()
{
	ofstream outFile(filename, ios::trunc);
	for (int i = 0; i <= 14; i++) {
		for (int j = 0; j <= 14; j++) {
			outFile << chessMap[i][j] << ' ';
		}outFile << endl;
	}outFile << game.mode;
	MessageBox(NULL, "已保存", " ", MB_OK);
}

bool Chess::clickBoard(int x, int y)
{
	int col = (x - margin_x) / chessSize;
	int row = (y - margin_y) / chessSize;

	float leftTopPosX = margin_x + chessSize * col;
	float leftTopPosY = margin_y + chessSize * row;
	float offset = chessSize * 0.5;
	double len;
	bool ret = 0;


	//左上角判断
	len = sqrt((x - leftTopPosX) * (x - leftTopPosX) +
		(y - leftTopPosY) * (y - leftTopPosY));
	if (len < offset) {
		pos.row = row;
		pos.col = col;
		if (chessMap[row][col] == 0 && row < 15 && row >= 0 && col < 15 && col >= 0) {
			return true;
		}
	}

	//右上角
	float x2 = leftTopPosX + chessSize;
	float y2 = leftTopPosY;
	len = sqrt((x - x2) * (x - x2) +
		(y - y2) * (y - y2));
	if (len < offset) {
		pos.row = row;
		pos.col = col + 1;
		if (chessMap[row][col + 1] == 0 && row < 15 && row >= 0 && col + 1 < 15 && col + 1 >= 0) {
			return true;
		}
	}

	//左下角
	x2 = leftTopPosX;
	y2 = leftTopPosY + chessSize;
	len = sqrt((x - x2) * (x - x2) +
		(y - y2) * (y - y2));
	if (len < offset) {
		if (chessMap[row + 1][col] == 0 && row + 1 < 15 && row + 1 >= 0 && col < 15 && col >= 0) {
			pos.row = row + 1;
			pos.col = col;
			return true;
		}
	}

	//右下角
	x2 = leftTopPosX + chessSize;
	y2 = leftTopPosY + chessSize;
	len = sqrt((x - x2) * (x - x2) +
		(y - y2) * (y - y2));
	if (len < offset) {
		if (chessMap[row + 1][col + 1] == 0 && row + 1 < 15 && row + 1 >= 0 && col + 1 < 15 && col + 1 >= 0) {
			pos.row = row + 1;
			pos.col = col + 1;
			return true;
		}
	}
	return false;
}

void Chess::chessDown(int row, int col, int kind)
{
	int x = margin_x + col * chessSize - 0.5 * chessSize;
	int y = margin_y + row * chessSize - 0.5 * chessSize;

	if (kind == -1) {
		loadimage(&chessWhiteImg, "source/white.png", chessSize, chessSize);
		putimagePNG(x, y, &chessWhiteImg);
	}
	else if (kind == 1) {
		loadimage(&chessBlackImg, "source/black.png", chessSize, chessSize);
		putimagePNG(x, y, &chessBlackImg);
	}
}

int Chess::checkWin()
{
	if (maxLength(1) == 5) {
		return 1;
	}
	if (maxLength(-1) == 5) {
		return -1;
	}
	return 0;
}

void Man::go(int kind)
{
	MOUSEMSG msg;
	while (1) {
		msg = GetMouseMsg();
		//通过chess对象，来调用 判断落子是否有效，以及落子功能
		if (msg.uMsg == WM_LBUTTONDOWN) {
			if (chess.clickBoard(msg.x, msg.y)) {
				//cout << msg.x << ' ' << msg.y << endl;
				//落子
				if (chess.ban(pos.row, pos.col)&& hint == 1 && game.mode == 1) {
					MessageBox(NULL, "禁手", "警告", MB_OK);
					continue;
				}
				if (chess.ban(pos.row, pos.col) && hint == -1 && game.mode == 1) {
					MessageBox(NULL, "你下了禁手\n电脑胜利", "游戏结束", MB_OK);
					game.play();
					break;
				}
				chess.chessDown(pos.row, pos.col, kind);
				chess.chessMap[pos.row][pos.col] = kind;
				break;
			}
			
			if (msg.x >= 50 && msg.x <= 250 && msg.y >= 726 && msg.y <= 766) {//认输
				MessageBox(NULL, "AI胜利", "游戏结束", MB_OK);

				ofstream outFile(filename, ios::trunc);
				for (int i = 0; i <= 14; i++) {
					for (int j = 0; j <= 14; j++) {
						chess.chessMap[i][j] = 0;
						outFile << chess.chessMap[i][j] << ' ';
					}outFile << endl;
				}
				outFile.close();
				chess.init();
				continue;
			}
			if (msg.x >= 350 && msg.x <= 550 && msg.y >= 666 && msg.y <= 706) {
				chess.save();
				continue;
			}
			if (msg.x >= 50 && msg.x <= 250 && msg.y >= 666 && msg.y <= 706) {
				hint *= -1;
				if (hint == 1) {
					IMAGE kai;
					loadimage(&kai, "source/开.png", 24, 27);
					putimagePNG(161, 670, &kai);
				}
				if (hint == -1) {
					IMAGE guan;
					loadimage(&guan, "source/关.png", 23, 29);
					putimagePNG(161, 670, &guan);
				}
				continue;
			}
			if (msg.x >= 350 && msg.x <= 550 && msg.y >= 726 && msg.y <= 766) {
				markEnd = 1;
				break;
			}
		}

	}
}

int Chess::surround(int l, int c) //周围8格有多少棋子
{
	int sum = 0;
	for (int u = 0; u < 8; u++) {
		if (chess.inboard(l + zong[u], c + heng[u]) && chess.chessMap[l + zong[u]][c + heng[u]] != 0) {
			sum++;
		}
	}
	return sum;
}

int AI::calculateScore(int l, int c) //根据活四，冲四，活三数等为每点重要性打分
{
	if (chess.chessMap[l][c] != 0) return -1;
	if (chess.ban(l, c) && game.mode == -1) {
		return -1;
	}

	else {
		int sum = chess.surround(l, c);
		for (int u = 0; u < 4; u++) {
			if (chess.samenum(l, c, u, -game.mode) + chess.samenum(l, c, u + 4, -game.mode) >= 4) {
				sum += 1000000; break;
			}
		}
		for (int u = 0; u < 4; u++) {
			if (chess.samenum(l, c, u, game.mode) + chess.samenum(l, c, u + 4, game.mode) >= 4) {
				sum += 100000; break;
			}
		}
		if (chess.ban(l, c) && game.mode == 1) {
			sum -= 50000;
		}
		return 10000 * chess.live4(l, c, -game.mode) + 200 * chess.live4(l, c, game.mode) + 55 * chess.chong4(l, c, -game.mode) + 60 * chess.chong4(l, c, game.mode) + 80 * chess.live3(l, c, -game.mode) + 40 * chess.live3(l, c, game.mode) + sum;
		//相同棋型AI方权重更高
	}
}

void AI::go(int kind)
{
	ChessPos result = ai.think();
	int row = result.row;
	int col = result.col;
	chess.chessDown(row, col, kind);
	chess.chessMap[row][col] = kind;
}

void AI::init()
{
	for (int i = 0; i < 16; i++) {
		for (int j = 0; j < 16; j++) {
			ai.scoreMap[i][j] = 0;
		}
	}
}

ChessPos AI::think()
{
	// 计算评分
	for (int i = 0; i <= 14; i++) {
		for (int j = 0; j <= 14; j++) {
			scoreMap[i][j] = calculateScore(i, j);
		}
	}
	for (int i = 0; i <= 14; i++) {
		for (int j = 0; j <= 14; j++) {


			// 从评分中找出最大分数的位置
			int maxScore = 0;
			vector<ChessPos> maxPoints;
			int k = 0;
			ChessPos temp;
			int size = 15;
			for (int row = 0; row < size; row++) {
				for (int col = 0; col < size; col++)
				{
					// 前提是这个坐标是空的
					if (chess.chessMap[row][col] == 0) {
						if (scoreMap[row][col] > maxScore)          // 找最大的数和坐标
						{
							maxScore = scoreMap[row][col];
							maxPoints.clear();
							temp.row = row;
							temp.col = col;
							maxPoints.push_back(temp);
						}
						else if (scoreMap[row][col] == maxScore) {   // 如果有多个最大的数，都存起来
							temp.row = row;
							temp.col = col;
							maxPoints.push_back(temp);
						}
					}
				}
			}


			// 随机落子，如果有多个点的话
			int index = rand() % maxPoints.size();
			return maxPoints[index];
		}
	}
}

void ChessGame::play()
{
	chess.init();
	while (1) {
		man.go(mode);
		if (markEnd == 1) {
			return;
		}
		if (chess.checkWin()) {
			if (chess.checkWin() == 1)
				MessageBox(NULL, "黑棋胜利", "游戏结束", MB_OK);
			else if (chess.checkWin() == -1)
				MessageBox(NULL, "白棋胜利", "游戏结束", MB_OK);
			else if (chess.checkWin() == -2)
				MessageBox(NULL, "由于禁手\n白棋胜利", "游戏结束", MB_OK);
			ofstream outFile(filename, ios::trunc);
			for (int i = 0; i <= 14; i++) {
				for (int j = 0; j <= 14; j++) {
					chess.chessMap[i][j] = 0;
					outFile << chess.chessMap[i][j] << ' ';
				}outFile << endl;
			}
			chess.init();
			continue;
		}
		ai.go(-mode);
		if (chess.checkWin()) {
			if (chess.checkWin() == 1)
				MessageBox(NULL, "黑棋胜利", "游戏结束", MB_OK);
			else if (chess.checkWin() == -1)
				MessageBox(NULL, "白棋胜利", "游戏结束", MB_OK);
			else if (chess.checkWin() == -2)
				MessageBox(NULL, "由于禁手\n白棋胜利", "游戏结束", MB_OK);
			ofstream outFile(filename, ios::trunc);
			for (int i = 0; i <= 14; i++) {
				for (int j = 0; j <= 14; j++) {
					chess.chessMap[i][j] = 0;
					outFile << chess.chessMap[i][j] << ' ';
				}outFile << endl;
			}
			chess.init();
			continue;
		}/*else if (mode == -1) {
			if (chess.judgeBegin()) {
				chess.chessDown(7, 7, 1);
			}
			else {
				ai.go(-mode);
				if (chess.checkWin()) {
					if (chess.checkWin() == 1)
						MessageBox(NULL, "黑棋胜利", "游戏结束", MB_OK);
					else if (chess.checkWin() == -1)
						MessageBox(NULL, "白棋胜利", "游戏结束", MB_OK);
					else if (chess.checkWin() == -2)
						MessageBox(NULL, "由于禁手\n白棋胜利", "游戏结束", MB_OK);
					ofstream outFile(filename, ios::trunc);
					for (int i = 0; i <= 14; i++) {
						for (int j = 0; j <= 14; j++) {
							chess.chessMap[i][j] = 0;
							outFile << chess.chessMap[i][j] << ' ';
						}outFile << endl;
					}
					chess.init();
					continue;
				}
			}
			
			man.go(mode);
			if (markEnd == 1) {
				return;
			}
			if (chess.checkWin()) {
				if (chess.checkWin() == 1)
					MessageBox(NULL, "黑棋胜利", "游戏结束", MB_OK);
				else if (chess.checkWin() == -1)
					MessageBox(NULL, "白棋胜利", "游戏结束", MB_OK);
				else if (chess.checkWin() == -2)
					MessageBox(NULL, "由于禁手\n白棋胜利", "游戏结束", MB_OK);
				ofstream outFile(filename, ios::trunc);
				for (int i = 0; i <= 14; i++) {
					for (int j = 0; j <= 14; j++) {
						chess.chessMap[i][j] = 0;
						outFile << chess.chessMap[i][j] << ' ';
					}outFile << endl;
				}
				chess.init();
				continue;
			}
		}*/
		
	}
	
}

int Chess::maxLength(int kind)
{
	int maxlen = 0;
	for (int i = 0; i <= 14; i++) {
		for (int j = 0; j <= 14; j++) {
			if (chessMap[i][j] == kind) {
				int ni = i;
				int nj = j;
				//横向
				int len = 0;
				while (chessMap[i][j] == kind) {
					len++;
					j++;
				}
				maxlen = max(maxlen, len);
				i = ni; j = nj;

				//纵向
				len = 0;
				while (chessMap[i][j] == kind) {
					len++;
					i++;
				}
				maxlen = max(maxlen, len);
				i = ni; j = nj;

				//左上-右下
				len = 0;
				while (chessMap[i][j] == kind) {
					len++;
					i++;
					j++;
				}
				maxlen = max(maxlen, len);
				i = ni; j = nj;

				//左下-右上
				len = 0;
				while (chessMap[i][j] == kind) {
					len++;
					i--;
					j++;
				}
				maxlen = max(maxlen, len);
				i = ni; j = nj;
			}
		}
	}
	return maxlen;
	return 0;
}

bool Chess::judgeBegin()
{
	for (int i = 0; i <= 14; i++) {
		for (int j = 0; j <= 14; j++) {
			if (chessMap[i][j] != 0) {
				return false;
			}
		}
	}
	return true;
}
int main() {
	chess.margin_x = 25;
	chess.margin_y = 25;
	chess.chessSize = 39.25;
	game.play();

	return 0;
}

