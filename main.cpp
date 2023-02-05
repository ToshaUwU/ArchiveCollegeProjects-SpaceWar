#include <graphics.h>
#include <cmath>
#include <ctime>
#include <string>

static constexpr double pi = 3.141592653589;
static const int DelayTime = 10;
static double TimeCounter = 0.018;
static const int ScreenWidth = 1024;
static const int ScreenHeight = 768;
static constexpr int ScreenHalfWidth = ScreenWidth/2;
static constexpr int ScreenHalfHeight = ScreenHeight/2;
static constexpr double SHHx075 = ScreenHalfHeight*0.75;
static const double Diagonal = sqrt(ScreenWidth*ScreenWidth + ScreenHeight*ScreenHeight);
static const double cos0_0005 = cos(0.0005);
static const double sin0_0005 = sin(0.0005);
static char sPlay[] = "Play";
static char sQuit[] = "Quit";
static char sResume[] = "Resume";
static char sRestart[] = "Restart";
static char sExit[] = "Exit";
static char sGamePaused[] = "Game Paused";
static char sYouLose[] = "You Lose";


#ifdef IncludeCosTable
static const unsigned int CosTableSize = 256;
static double * CosTable;
static constexpr double ReversedTwoPi = CosTableSize/(2.0*pi);
#include <iostream>
static inline void InitCosTable()
{
	CosTable = new double[CosTableSize];
	constexpr double CosUnit = 1.0/ReversedTwoPi;
	for(unsigned int i = 0; i < CosTableSize; i++)
		std::cout << (CosTable[i] = cos(CosUnit*i)) << std::endl;
}
static inline void ClearCosTable(){delete [] CosTable;}
static inline double fcos(double x){return CosTable[static_cast<unsigned int>(x*ReversedTwoPi + 0.5) % 256];}
static inline double fsin(double x){return fcos(pi/2.0 - x);}
#else
#define fcos(x) cos(x)
#define fsin(x) sin(x)
#endif

static inline int random(int a, int b){return (rand() % (b - a + 1) + a);}
static inline double random(double a, double b){return (rand()*(b - a)/RAND_MAX + a);}
static inline bool SpawnChance(double startChance, double increasePerMinute, double Time, int CountReduce){ double Chance = (startChance + increasePerMinute*(Time/60.0))/(1000.0/DelayTime);
																											return !random(0,static_cast<int>((100-Chance)/Chance)*CountReduce);}

template <typename T>
static inline T max(T a, T b){return a > b? a: b;}
template <typename T>
static inline T min(T a, T b){return a < b? a: b;}

enum GameState{GameEnded, GameInProcess, GamePaused, GameRestarting, LoseProcessed};
static bool menuProcess(int EndGameState);
static int pauseProcess();
static int loseProcess();
static void DrawGui(double Health, double Time, int Energy, double EnergyGraph[30], unsigned int Kills, double k, bool LoseBulb);
static void ClearInput();
static char * numberToString(unsigned int Num, char * Str);
static char * numberToString(double Num, char * Str, unsigned int precision);
static char * ConvertTime(double Num, char * Str, unsigned int precision = 2);

struct Point
{
	double x;
	double y;

	void SetDots(double x_, double y_){x = x_, y = y_;}
	void MovePoint(double Angle, double Distance)
	{
		x += fcos(Angle) * Distance;
		y += fsin(-Angle) * Distance;
	}

	Point() = default;
	Point(Point & Data): x(Data.x), y(Data.y){}
};

template<unsigned int Size>
class counter
{
private:
	unsigned int x : Size;

public:
	counter(): x(0){}
	counter(unsigned int y): x(y){}
	operator bool(){return x < ((1 << Size) >> 1);}
	void operator++(int){x++;}
	counter & operator=(unsigned int y){return (x = y, *this);}
};

template<typename T>
class List
{
private:

	struct Node
	{
		T Data;
		Node * PrevNode;
		Node * NextNode;

		Node(T & Data_): Data(Data_), PrevNode(nullptr), NextNode(nullptr){}
	};

	Node * First;
	Node * Last;

	void DeleteNode(Node * NodeForDeletion);

public:
	operator=(List &) = delete;
	operator=(List &&) = delete;
	List(List &) = delete;
	List(List &&) = delete;

	List(): First(nullptr), Last(nullptr){}
	void CreateNode(T && Data);
	void ForEach(void (*Action)(T & Data));
	unsigned int CountIf(bool (*Check)(T & Data));
	void CheckForDelete(bool (*Check)(T & Data));
	unsigned int DeleteAndCount(bool (*Check)(T & Data));
	Node * GetFirstPtr(){return First;}
	template<typename T2>
	void CompareWith(List<T2> & List2, void(*Action)(T & Data1, T2 & Data2));
	template<typename... Types>
	void CompareWith(Types&... Args, void(*Action)(Types&... Args, T & Data));
	void Clear();
	~List();
};

template<typename T>
void List<T>::CreateNode(T && Data)
{
	if(First)
	{
		Last->NextNode = new Node(Data);
		Last->NextNode->PrevNode = Last;
		Last = Last->NextNode;
	}
	else
		First = Last = new Node(Data);
}

template<typename T>
void List<T>::DeleteNode(Node * NodeForDeletion)
{
	if(NodeForDeletion == First)
		First = NodeForDeletion->NextNode;
	else
		NodeForDeletion->PrevNode->NextNode = NodeForDeletion->NextNode;
	if(NodeForDeletion == Last)
		Last = NodeForDeletion->PrevNode;
	else
		NodeForDeletion->NextNode->PrevNode = NodeForDeletion->PrevNode;
	delete NodeForDeletion;
}

template<typename T>
void List<T>::ForEach(void (*Action)(T & Data))
{
	for(Node * i = First; i; i = i->NextNode)
		Action(i->Data);
}

template<typename T>
template<typename T2>
void List<T>::CompareWith(List<T2> & List2, void(*Action)(T & Data1, T2 & Data2))
{
	for(Node * i = First; i; i = i->NextNode)
		for(decltype(List2.GetFirstPtr()) j = List2.GetFirstPtr(); j; j = j->NextNode)
			Action(i->Data, j->Data);
}

template<typename T>
template<typename... Types>
void List<T>::CompareWith(Types&... Args, void(*Action)(Types&... Args, T & Data))
{
	for(Node * i = First; i; i = i->NextNode)
		Action(Args... , i->Data);
}

template<typename T>
unsigned int List<T>::CountIf(bool (*Check)(T & Data))
{
	unsigned int Count = 0;
	for(Node * i = First; i; i = i->NextNode)
		Count += Check(i->Data);
	return Count;
}

template<typename T>
unsigned int List<T>::DeleteAndCount(bool (*Check)(T & Data))
{
	unsigned int Count = 0;
	for(Node * i = First; i; i = i->NextNode)
		if(Check(i->Data))
		{
			DeleteNode(i);
			Count++;
		}
	return Count;
}

template<typename T>
void List<T>::CheckForDelete(bool (*Check)(T & Data))
{
	Node * Temp;
	for(Node * i = First; i; i = Temp)
	{
		Temp = i->NextNode;
		if(Check(i->Data))
			DeleteNode(i);
	}
}

template<typename T>
void List<T>::Clear()
{
	Node * temp;
	while(First)
	{
		temp = First->NextNode;
		delete First;
		First = temp;
	}
}

template<typename T>
List<T>::~List<T>()
{
	Node * temp;
	while(First)
	{
		temp = First->NextNode;
		delete First;
		First = temp;
	}
}

class BulletsArray
{
public:

	struct Bullet
	{
		Point BulletDot;
		Point Tail;
		double Angle;
		enum{DeleteNow = 5};
		int Deletion;

		Bullet(double x, double y, double Angle_): Angle(Angle_), Deletion(0){BulletDot.SetDots(x,y), BulletDot.MovePoint(Angle_, 15.0), Tail.SetDots(x,y);}
		Bullet(Bullet & Data): Angle(Data.Angle), Deletion(Data.Deletion){BulletDot.SetDots(Data.BulletDot.x, Data.BulletDot.y), BulletDot.MovePoint(Angle, 15.0), Tail.SetDots(Data.Tail.x, Data.Tail.y);}
	};

private:

	int Color[3];
	int Thickness;
	double Speed;

	List<Bullet> Bullets;

public:

	BulletsArray(int r, int g, int b, double Speed_, int Thickness_ = 1): Thickness(Thickness_), Speed(Speed_){Color[0] = r, Color[1] = g, Color[2] = b;}
	List<Bullet> & GetBulletsList(){return Bullets;}
	void CreateBullet(double x, double y, double Angle){Bullets.CreateNode(Bullet(x, y, Angle));}
	void MoveBullets();
	void DrawBullets();
	void CheckForDeletion();
	void DeleteAll(){Bullets.Clear();}
};

void BulletsArray::CheckForDeletion()
{
	Bullets.CheckForDelete([](Bullet & Data){return Data.Deletion == Bullet::DeleteNow;});
}

void BulletsArray::MoveBullets()
{
	void (*Action)(double &, Bullet &) = [](double & Speed, Bullet & Data)
																		{
																			if(!Data.Deletion)
																			{
																				Data.BulletDot.MovePoint(Data.Angle, Speed);
																				Data.Tail.MovePoint(Data.Angle, Speed);
																			}
																			else
																			{
																				Data.Deletion++;
																				Data.Tail.MovePoint(Data.Angle, Speed/1.5);
																			}
																				if(Data.Tail.x < -200 || Data.Tail.x > ScreenWidth + 200 || Data.Tail.y < -200 || Data.Tail.y > ScreenHeight + 200)
																					Data.Deletion = 5;
																		};
	Bullets.CompareWith<double>(Speed, Action);
}

void BulletsArray::DrawBullets()
{
	setcolor(COLOR(Color[0], Color[1], Color[2]));
	setfillstyle(SOLID_FILL, COLOR(Color[0], Color[1], Color[2]));
	void (*Action)(int &, Bullet &) = [](int & Thickness, Bullet & Data)
																		{
																			setlinestyle(SOLID_LINE, 0, Thickness);
																			moveto(Data.Tail.x, Data.Tail.y);
																			lineto(Data.BulletDot.x, Data.BulletDot.y);
																			setlinestyle(SOLID_LINE, 0, 1);
																			if(Data.Deletion)
																				fillellipse(Data.BulletDot.x, Data.BulletDot.y, Data.Deletion*(Thickness > 4? 4: Thickness), Data.Deletion*(Thickness > 4? 4: Thickness));
																		};
	Bullets.CompareWith<int>(Thickness, Action);
}

class Ship;

template<int DotsCount, int ControlDots, typename... DoActionTypes>
class Enemy
{
protected:

	Point Center;
	double Angle;
	Point Dots[DotsCount];
	unsigned int Shape[ControlDots];
	int CoolDown;
	double Health;
	int State;
	enum{Left, Right, Up, Down};
	int Dead;
	double Damage;

	void MoveEnemy(double Speed);
	virtual void DrawHealthBar() = 0;

public:

	enum{DeadRightNow = 10};

	Enemy();
	Enemy(Enemy & Data);
	virtual void DoAction(DoActionTypes... PassedData) = 0;
	void TakeDamage(){Health = max(Health - Damage, 0.0);}
	void TakeDamage(double HowMany){Health = max(Health - HowMany, 0.0);}
	void DrawEnemy();
	int GetState(){return State;}
	bool DotIn(double x, double y);
	bool IsAlive(){return Health > 0.0 && !Dead;}
	int & GetDeadClock(){return Dead;}
	virtual ~Enemy() = default;
};

template<int DotsCount, int ControlDots, typename... DoActionTypes>
void Enemy<DotsCount, ControlDots, DoActionTypes...>::MoveEnemy(double Speed)
{
	Center.MovePoint(Angle, Speed);
	for(int i = 0; i < DotsCount; i++)
		Dots[i].MovePoint(Angle, Speed);
}

template<int DotsCount, int ControlDots, typename... DoActionTypes>
Enemy<DotsCount, ControlDots, DoActionTypes...>::Enemy()
{
	switch(random(Left, Down))
	{
	case Left:
		Center.SetDots(-50.0, random(101, ScreenHeight - 101));
		Angle = 0.0;
		break;

	case Right:
		Center.SetDots(ScreenWidth + 50.0, random(101, ScreenHeight - 101));
		Angle = pi;
		break;

	case Up:
		Center.SetDots(random(51, ScreenWidth - 51), -50.0);
		Angle = pi*3.0/2.0;
		break;

	case Down:
		Center.SetDots(random(51, ScreenWidth - 51), ScreenHeight + 50.0);
		Angle = pi/2.0;
		break;
	}
	CoolDown = 30;
	Health = 100.0;
	Dead = 0;
	State = 0;
	Damage = 10.0;
}

template<int DotsCount, int ControlDots, typename... DoActionTypes>
Enemy<DotsCount, ControlDots, DoActionTypes...>::Enemy(Enemy & Data)
{
	Center.SetDots(Data.Center.x, Data.Center.y);
	Angle = Data.Angle;
	for(int i = 0; i < DotsCount; i++)
		Dots[i].SetDots(Data.Dots[i].x, Data.Dots[i].y);
	for(int i = 0; i < ControlDots; i++)
		Shape[i] = Data.Shape[i];
	CoolDown = Data.CoolDown;
	Health = Data.Health;
	State = Data.State;
	Dead = Data.Dead;
	Damage = Data.Damage;
}

template<int DotsCount, int ControlDots, typename... DoActionTypes>
void Enemy<DotsCount, ControlDots, DoActionTypes...>::DrawEnemy()
{
	double Tempx, Tempy;
	if(!Dead)
	{
		int DotsBuf[DotsCount*2];
		for(int i = 0; i < DotsCount; i++)
		{
			Tempx = Dots[i].x - Center.x;
			Tempy = Dots[i].y - Center.y;
			DotsBuf[i*2] = Tempx*fcos(Angle) - Tempy*fsin(-Angle) + Center.x;
			DotsBuf[i*2 + 1] = Tempx*fsin(-Angle) + Tempy*fcos(Angle) + Center.y;
		}
		setfillstyle(SOLID_FILL, COLOR(128, 0, 0));
		setcolor(COLOR(255, 0, 0));
		fillpoly(DotsCount, DotsBuf);

		if(Health < 100.0)
		{
			setfillstyle(SOLID_FILL, COLOR(255 * min((100.0 - Health)/50.0, 1.0), 255 * min(Health/50.0, 1.0), 0));
			DrawHealthBar();
		}
	}
	else
	{
		setcolor(COLOR(255, 64, 0));
		setfillstyle(SOLID_FILL, COLOR(255, 128, 0));
		fillellipse(Center.x, Center.y, Dead*3, Dead*3);
	}
}

template<int DotsCount, int ControlDots, typename... DoActionTypes>
bool Enemy<DotsCount, ControlDots, DoActionTypes...>::DotIn(double x, double y)
{
	double Tempx, Tempy;
	Point DotsBuf[ControlDots];
	for(int i = 0; i < ControlDots; i++)
	{
		Tempx = Dots[Shape[i]].x - Center.x;
		Tempy = Dots[Shape[i]].y - Center.y;
		DotsBuf[i].x = Tempx*fcos(Angle) - Tempy*fsin(-Angle) + Center.x;
		DotsBuf[i].y = Tempx*fsin(-Angle) + Tempy*fcos(Angle) + Center.y;
	}

	int Check = 0;

	for(int i = 0; i < ControlDots - 1; i++)
		if((DotsBuf[i+1].x - DotsBuf[i].x)*(y - DotsBuf[i].y) - (DotsBuf[i+1].y - DotsBuf[i].y)*(x - DotsBuf[i].x) >= 0)
			Check++;

	if((DotsBuf[0].x - DotsBuf[ControlDots - 1].x)*(y - DotsBuf[ControlDots - 1].y) - (DotsBuf[0].y - DotsBuf[ControlDots - 1].y)*(x - DotsBuf[ControlDots - 1].x) >= 0)
		Check++;

	if(Check == ControlDots)
		return true;
	else
		return false;
}

template<class EnemyType>
class EnemyList
{
private:

	int MaxEnemys;
	int EnemysAlive;
	List<EnemyType> Enemys;

public:

	EnemyList(int Max): MaxEnemys(Max), EnemysAlive(0){}
	void SpawnEnemy();
	template<typename... DoActionTypes>
	void ProcessEnemys(DoActionTypes... DoActionData);
	void DrawEnemys(){Enemys.ForEach([](EnemyType & Data){Data.DrawEnemy();});}
	int GetEnemysCount(){return EnemysAlive;}
	int CheckForDead();
	void CheckForHits(BulletsArray & BulletsForCheck);
	void DeleteAll(){Enemys.Clear();EnemysAlive = 0;}
};

template<class EnemyType>
void EnemyList<EnemyType>::SpawnEnemy()
{
	if(MaxEnemys > EnemysAlive)
	{
		Enemys.CreateNode(EnemyType());
		EnemysAlive++;
	}
}

template<class EnemyType>
int EnemyList<EnemyType>::CheckForDead()
{
	int EnemysAliveNow = EnemysAlive;
	void (*Action)(EnemyType &) = [](EnemyType & Data)
													{
														if(!Data.IsAlive())
															Data.GetDeadClock()++;
													};
	Enemys.ForEach(Action);
	EnemysAlive -= Enemys.DeleteAndCount([](EnemyType & Data){return Data.GetDeadClock() == EnemyType::DeadRightNow;});
	return EnemysAliveNow - EnemysAlive;
}

template<class EnemyType>
void EnemyList<EnemyType>::CheckForHits(BulletsArray & BulletsForCheck)
{
	void (*Action)(EnemyType &, BulletsArray::Bullet &) = [](EnemyType & Data1, BulletsArray::Bullet & Data2)
																											{
																												if(Data1.DotIn(Data2.BulletDot.x, Data2.BulletDot.y) && !Data2.Deletion)
																												{
																													Data1.TakeDamage();
																													Data2.Deletion = 1;
																												}
																											};
	Enemys.CompareWith(BulletsForCheck.GetBulletsList(), Action);
}


class Bull: public Enemy<4, 4, double, double, bool, Ship & >
{
private:

	double BurstLength;
	double PassedWay;

	virtual void DrawHealthBar();
	void CheckForDamage(Ship & Player);

public:

	enum{MoveToField, Stay, Burst};

	Bull();
	Bull(Bull & Data): Enemy<4, 4, double, double, bool, Ship & >(Data), BurstLength(Data.BurstLength), PassedWay(Data.PassedWay){}
	virtual void DoAction(double x, double y, bool PlayerAlive, Ship & Player);
	virtual ~Bull() = default;
};

void inline Bull::DrawHealthBar()
{
	bar(Center.x - 25, Center.y - 25 - 10 * std::abs(fsin(Angle)), Center.x - 25 + 50*(Health/100.0), Center.y - 30 - 10 * std::abs(fsin(Angle)));
}

Bull::Bull(): Enemy<4, 4, double, double, bool, Ship & >(), BurstLength(0.0), PassedWay(0.0)
{
	Dots[0].SetDots(Center.x + 25.0, Center.y);
	Dots[1].SetDots(Center.x - 15.0, Center.y + 16.0);
	Dots[2].SetDots(Center.x - 25.0, Center.y);
	Dots[3].SetDots(Center.x - 15.0, Center.y - 16.0);
	for(int i = 0; i < 4; i++)
		Shape[i] = i;
}

template<>
template<>
void EnemyList<Bull>::ProcessEnemys(double x, double y, bool PlayerAlive, Ship & Player)
{
	void (*Action)(double &, double &, bool &, Ship &, Bull &) = [](double & x, double & y, bool & PlayerAlive, Ship & Player, Bull & Data){Data.DoAction(x, y, PlayerAlive, Player);};
	Enemys.CompareWith<double, double, bool, Ship>(x, y, PlayerAlive, Player, Action);
}

void Bull::DoAction(double x, double y, bool PlayerAlive, Ship & Player)
{
	if(!IsAlive())
		return;

	switch(State)
	{
	case MoveToField:
		if(Center.x < 50 || Center.x > ScreenWidth - 50 || Center.y < 100 || Center.y > ScreenHeight - 100)
			MoveEnemy(2.0);
		else
			State = Stay;
		break;

	case Stay:
		{
			double vx = x - Center.x, vy = y - Center.y, Length;
			Length = sqrt(vx*vx + vy*vy);
			Angle = acos(vx / Length) * (vy > 0.0? -1.0: 1.0) + 2.0*pi;
			if(CoolDown == 0 && PlayerAlive)
			{
				BurstLength = Length * 1.2;
				State = Burst;
				CoolDown = random(50, 200);
				PassedWay = 0.0;
				Angle += random(-0.05, 0.05);
			}
			else
				CoolDown--;
		}
		break;

	case Burst:
		if(BurstLength > PassedWay)
		{
			MoveEnemy(12.0);
			PassedWay += sqrt(fcos(Angle)*fcos(Angle) + fsin(-Angle)*fsin(-Angle))*12.0;
		}
		else
			if(PlayerAlive)
				State = Stay;
		CheckForDamage(Player);
	}
}

class Turret: public Enemy<10, 6, double, double, bool, BulletsArray &>
{
private:

	virtual void DrawHealthBar();

public:

	enum{MoveToField, Shooting};

	Turret();
	Turret(Turret & Data): Enemy<10, 6, double, double, bool, BulletsArray &>(Data){}
	virtual void DoAction(double x, double y, bool PlayerAlive, BulletsArray & EnemeyBullets);
	virtual ~Turret() = default;
};

void inline Turret::DrawHealthBar()
{
	bar(Center.x - 25, Center.y - 37, Center.x - 25 + 50*(Health/100.0), Center.y - 42);
}

Turret::Turret(): Enemy<10, 6, double, double, bool, BulletsArray &>()
{
	Dots[0].SetDots(Center.x + 15.0, Center.y);
	Dots[1].SetDots(Center.x - 7.5, Center.y - 10.5);
	Dots[2].SetDots(Center.x, Center.y - 18.0);
	Dots[3].SetDots(Center.x + 22.5, Center.y - 12.0);
	Dots[4].SetDots(Center.x - 4.5, Center.y - 30.0);
	Dots[5].SetDots(Center.x - 30.0, Center.y);
	Dots[6].SetDots(Center.x - 4.5, Center.y + 30.0);
	Dots[7].SetDots(Center.x + 22.5, Center.y + 12.0);
	Dots[8].SetDots(Center.x, Center.y + 18.0);
	Dots[9].SetDots(Center.x - 7.5, Center.y + 10.5);
	Shape[0] = 7;
	Shape[1] = 6;
	Shape[2] = 5;
	Shape[3] = 4;
	Shape[4] = 3;
	Shape[5] = 0;
	Damage = 5.0;
}

template<>
template<>
void EnemyList<Turret>::ProcessEnemys(double x, double y, bool PlayerAlive, BulletsArray & EnemyBullets)
{
	void (*Action)(double &, double &, bool &, BulletsArray &, Turret &) = [](double & x, double & y, bool & PlayerAlive, BulletsArray & EnemyBullets, Turret & Data){Data.DoAction(x, y, PlayerAlive, EnemyBullets);};
	Enemys.CompareWith<double, double, bool, BulletsArray>(x, y, PlayerAlive, EnemyBullets, Action);
}

void Turret::DoAction(double x, double y, bool PlayerAlive, BulletsArray & EnemeyBullets)
{
	if(!IsAlive())
		return;
	switch(State)
	{
	case MoveToField:
		if(Center.x < 50 || Center.x > ScreenWidth - 50 || Center.y < 100 || Center.y > ScreenHeight - 100)
			MoveEnemy(2.0);
		else
			State = Shooting;
		break;

	case Shooting:
		{
			double vx = x - Center.x, vy = y - Center.y;
			Angle = acos(vx / sqrt(vx*vx + vy*vy)) * (vy > 0.0? -1.0: 1.0) + 2.0*pi;
			if(CoolDown == 0 && PlayerAlive)
			{
				Point DotsBuf;
				double Tempx = Dots[0].x - Center.x;
				double Tempy = Dots[0].y - Center.y;
				DotsBuf.x = Tempx*fcos(Angle) - Tempy*fsin(-Angle) + Center.x;
				DotsBuf.y = Tempx*fsin(-Angle) + Tempy*fcos(Angle) + Center.y;
				EnemeyBullets.CreateBullet(DotsBuf.x, DotsBuf.y, Angle + random(-0.05, 0.05));
				CoolDown = random(50, 175);
			}
			else
				CoolDown--;
		}
		break;
	}
}

class LaserWall: public Enemy<7, 5, double, double, bool, BulletsArray &>
{
private:

	Point NewPosition;
	counter<1> ShootCnt;

	virtual void DrawHealthBar();

public:

	enum{MoveToField, Stay, Prepare, Shooting, Redislocation};

	LaserWall();
	LaserWall(LaserWall & Data): Enemy<7, 5, double, double, bool, BulletsArray &>(Data){}
	virtual void DoAction(double x, double y, bool PlayerAlive, BulletsArray & LaserBullets);
	virtual ~LaserWall() = default;

};

void LaserWall::DrawHealthBar()
{
	bar(Center.x - 25, Center.y - 37, Center.x - 25 + 50*(Health/100.0), Center.y - 42);
}

LaserWall::LaserWall()
{
	Dots[0].SetDots(Center.x, Center.y - 10.0);
	Dots[1].SetDots(Center.x + 30.0, Center.y - 10.0);
	Dots[2].SetDots(Center.x, Center.y - 30.0);
	Dots[3].SetDots(Center.x - 30.0, Center.y);
	Dots[4].SetDots(Center.x, Center.y + 30.0);
	Dots[5].SetDots(Center.x + 30.0, Center.y + 10.0);
	Dots[6].SetDots(Center.x, Center.y + 10.0);
	Shape[0] = 5;
	Shape[1] = 4;
	Shape[2] = 3;
	Shape[3] = 2;
	Shape[4] = 1;
	Damage = 3.5;
}

template<>
template<>
void EnemyList<LaserWall>::ProcessEnemys(double x, double y, bool PlayerAlive, BulletsArray & LaserBullets)
{
	void (*Action)(double &, double &, bool &, BulletsArray &, LaserWall &) = [](double & x, double & y, bool & PlayerAlive, BulletsArray & LaserBullets, LaserWall & Data){Data.DoAction(x, y, PlayerAlive, LaserBullets);};
	Enemys.CompareWith<double, double, bool, BulletsArray &>(x, y, PlayerAlive, LaserBullets, Action);
}

void LaserWall::DoAction(double x, double y, bool PlayerAlive, BulletsArray & LaserBullets)
{
	if(!IsAlive())
		return;
	switch(State)
	{
	case MoveToField:
		if(Center.x < 50 || Center.x > ScreenWidth - 50 || Center.y < 100 || Center.y > ScreenHeight - 100)
			MoveEnemy(2.0);
		else
		{
			CoolDown = 50;
			State = Stay;
		}
		break;

	case Stay:
		{
			double vx = x - Center.x, vy = y - Center.y;
			Angle = acos(vx / sqrt(vx*vx + vy*vy)) * (vy > 0.0? -1.0: 1.0) + 2.0*pi;
			if(CoolDown == 0 && PlayerAlive)
			{
				CoolDown = 30;
				State = Prepare;
			}
			else
				CoolDown--;
		}
		break;

	case Prepare:
			if(CoolDown == 0 && PlayerAlive)
			{
				CoolDown = 350;
				State = Shooting;
			}
			else
				CoolDown--;
		break;

	case Shooting:
			if(ShootCnt && PlayerAlive)
				LaserBullets.CreateBullet(Center.x, Center.y, Angle);
			ShootCnt++;
			if(CoolDown == 0 && PlayerAlive)
			{
				NewPosition.SetDots(random(50, ScreenWidth - 50), random(100, ScreenHeight - 100));
				double vx = NewPosition.x - Center.x, vy = NewPosition.y - Center.y;
				Angle = acos(vx / sqrt(vx*vx + vy*vy)) * (vy > 0.0? -1.0: 1.0) + 2.0*pi;
				State = Redislocation;
			}
			else
				CoolDown--;
		break;

	case Redislocation:
		MoveEnemy(4.0);
		if(std::abs(NewPosition.x - Center.x) + std::abs(NewPosition.y - Center.y) < 16)
		{
			CoolDown = random(50, 100);
			State = Stay;
		}
		break;
	}
}

class Ship
{
private:

	Point Center;
	double Angle;
	Point Dots[4];
	counter<1> ShooterCnt;
	counter<3> Flick;
	int DamageCoolDown;
	int CoolDown;
	Point Shooters[2];
	double Speed;
	double MaxSpeed;
	int Acceleration;
	double Health;
	double Energy;
	bool GodMode;
	bool InfinityEnergy;
	int Stop;

	bool DotIn(double x, double y);

public:
	Ship();
	enum{Right = -1, Ahead, Left};
	void MoveShip(int Where);
	void GetDots(double Dots[8]);
	void GetCenter(double xy[2]){xy[0] = Center.x, xy[1] = Center.y;}
	enum{Center_x, Center_y};
	double GetCenter(int x_or_y){return x_or_y? Center.y: Center.x;}
	bool GetDots(int Dots[8]);
	void CalcAcceleration();
	enum{SpeedDown, SpeedUp};
	void SetAcceleration(bool AccState){Acceleration = AccState;}
	void SetAngle(int x, int y);
	double GetHealth(){return GodMode? -1.0: Health;}
	double GetEnergy(){return InfinityEnergy? -1.0: Energy;}
	void Shoot(BulletsArray & Bullets);
	void Reset();
	void EnergyRegenerate(){Energy = min(Energy + 0.175, 100.0);}
	void HealthRegenerate(){Health = min(Health + 0.01, 100.0);}
	void TakeDamage(double HowMany = 25.0){Health = max(Health - HowMany, 0.0);DamageCoolDown = 0;}
	bool IsInvincible(){return (DamageCoolDown < 50) || GodMode;}
	bool IsAlive(){return Health;}
	void RefreshCoolDown();
	void SetGodMode();
	void SetInfinityEnergy();
	void CheckForHits(BulletsArray & EnemyBullets, double HowManyDamageOccur = 10.0);
};

void Bull::CheckForDamage(Ship & Player)
{
	double Dots[8], Center[2];
	Player.GetDots(Dots);
	Player.GetCenter(Center);
	if(Player.IsInvincible())
		return;
	if(DotIn(Dots[0], Dots[1]) || DotIn(Dots[2], Dots[3]) || DotIn(Dots[4], Dots[5]) || DotIn(Dots[6], Dots[7]) || DotIn(Center[0], Center[1]))
	{
		Player.TakeDamage();
		TakeDamage(50.0);
	}
}

void Ship::RefreshCoolDown()
{
	CoolDown = min(CoolDown + 1, 5);
	DamageCoolDown = min(DamageCoolDown + 1, 50);
	if(DamageCoolDown < 50)
		Flick++;
	else
		Flick = 0;
}

void Ship::SetGodMode()
{
	GodMode = !GodMode;
	Health = 100.0;
}

void Ship::SetInfinityEnergy()
{
	InfinityEnergy = !InfinityEnergy;
	Energy = 100.0;
}

Ship::Ship()
{
	Center.SetDots(ScreenHalfWidth, ScreenHalfHeight);
	Angle = pi/2.0;
	Dots[0].SetDots(Center.x + 25.0, Center.y);
	Dots[1].SetDots(Center.x - 15.0, Center.y + 16.0);
	Dots[2].SetDots(Center.x - 10.0, Center.y);
	Dots[3].SetDots(Center.x - 15.0, Center.y - 16.0);
	ShooterCnt = 0;
	Shooters[0].SetDots(Center.x - 2, Center.y - 7);
	Shooters[1].SetDots(Center.x - 2, Center.y + 7);
	Speed = 0.0;
	MaxSpeed = 6.0;
	Acceleration = false;
	Health = 100.0;
	Energy = 100.0;
	CoolDown = 5;
	GodMode = false;
	InfinityEnergy = false;
	Flick = 0;
	DamageCoolDown = 50;
	Stop = 0;
}

void Ship::Reset()
{
	Center.SetDots(ScreenHalfWidth, ScreenHalfHeight);
	Angle = pi/2.0;
	Dots[0].SetDots(Center.x + 25.0, Center.y);
	Dots[1].SetDots(Center.x - 15.0, Center.y + 16.0);
	Dots[2].SetDots(Center.x - 10.0, Center.y);
	Dots[3].SetDots(Center.x - 15.0, Center.y - 16.0);
	ShooterCnt = 0;
	Shooters[0].SetDots(Center.x - 2, Center.y - 7);
	Shooters[1].SetDots(Center.x - 2, Center.y + 7);
	Speed = 0.0;
	MaxSpeed = 6.0;
	Acceleration = false;
	Health = 100.0;
	Energy = 100.0;
	CoolDown = 5;
	GodMode = false;
	InfinityEnergy = false;
	Flick = 0;
	DamageCoolDown = 50;
	Stop = 0;
}

void Ship::CalcAcceleration()
{
	if(Acceleration)
		Speed = MaxSpeed;
	else
		Speed = max(Speed - 0.1, 0.0);
}

void Ship::Shoot(BulletsArray & Bullets)
{
	if(CoolDown < 5 || Energy < 3.5)
		return;
	double Resultx, Resulty;
	Resultx = (Shooters[ShooterCnt].x - Center.x)*fcos(Angle) - (Shooters[ShooterCnt].y - Center.y)*fsin(-Angle) + Center.x;
	Resulty = (Shooters[ShooterCnt].x - Center.x)*fsin(-Angle) + (Shooters[ShooterCnt].y - Center.y)*fcos(Angle) + Center.y;
	Bullets.CreateBullet(Resultx, Resulty, Angle);
	ShooterCnt++;
	if(!InfinityEnergy)
		Energy = max(Energy - 3.5, 1.0);
	CoolDown = 0;
}

bool Ship::DotIn(double x, double y)
{
	double Tempx, Tempy;
	Point DotsBuf[4];
	for(int i = 0; i < 4; i++)
	{
		Tempx = Dots[i].x - Center.x;
		Tempy = Dots[i].y - Center.y;
		DotsBuf[i].x = Tempx*fcos(Angle) - Tempy*fsin(-Angle) + Center.x;
		DotsBuf[i].y = Tempx*fsin(-Angle) + Tempy*fcos(Angle) + Center.y;
	}
	if((DotsBuf[1].x - DotsBuf[0].x)*(y - DotsBuf[0].y) - (DotsBuf[1].y - DotsBuf[0].y)*(x - DotsBuf[0].x) >= 0.0 &&
	   (DotsBuf[2].x - DotsBuf[1].x)*(y - DotsBuf[1].y) - (DotsBuf[2].y - DotsBuf[1].y)*(x - DotsBuf[1].x) >= 0.0 &&
	   (DotsBuf[3].x - DotsBuf[2].x)*(y - DotsBuf[2].y) - (DotsBuf[3].y - DotsBuf[2].y)*(x - DotsBuf[2].x) >= 0.0 &&
	   (DotsBuf[0].x - DotsBuf[3].x)*(y - DotsBuf[3].y) - (DotsBuf[0].y - DotsBuf[3].y)*(x - DotsBuf[3].x) >= 0.0)
			return true;
	return false;
}

void Ship::CheckForHits(BulletsArray & EnemyBullets, double HowManyDamageOccur)
{
	void (*Action)(Ship &, double &, BulletsArray::Bullet &) = [](Ship & Player, double & HowManyDamageOccur, BulletsArray::Bullet & Bullet)
																																			{
																																				if(Player.DotIn(Bullet.BulletDot.x, Bullet.BulletDot.y) && !Bullet.Deletion)
																																				{
																																					if(!Player.IsInvincible())
																																						Player.TakeDamage(HowManyDamageOccur);
																																					Bullet.Deletion = 1;
																																				}
																																			};
	EnemyBullets.GetBulletsList().CompareWith<Ship, double>(*this, HowManyDamageOccur, Action);
}

void Ship::SetAngle(int x, int y)
{
	double vx = x - Center.x, vy = y - Center.y;
	Angle = acos(vx / sqrt(vx*vx + vy*vy)) * (vy > 0.0? -1.0: 1.0);
}

void Ship::MoveShip(int Where)
{
	double NewAngle = Angle + pi/2.0 * Where;
	Center.MovePoint(NewAngle, Speed);
	Dots[0].MovePoint(NewAngle, Speed);
	Dots[1].MovePoint(NewAngle, Speed);
	Dots[2].MovePoint(NewAngle, Speed);
	Dots[3].MovePoint(NewAngle, Speed);
	Shooters[0].MovePoint(NewAngle, Speed);
	Shooters[1].MovePoint(NewAngle, Speed);
}

void Ship::GetDots(double DotsBuf[8])
{
	double Tempx, Tempy;
	for(int i = 0; i < 4; i++)
	{
		Tempx = Dots[i].x - Center.x;
		Tempy = Dots[i].y - Center.y;
		DotsBuf[i*2] = Tempx*fcos(Angle) - Tempy*fsin(-Angle) + Center.x;
		DotsBuf[i*2 + 1] = Tempx*fsin(-Angle) + Tempy*fcos(Angle) + Center.y;
	}
}

bool Ship::GetDots(int DotsBuf[8])
{
	if(!Flick)
		return false;

	double Tempx, Tempy;
	for(int i = 0; i < 4; i++)
	{
		Tempx = Dots[i].x - Center.x;
		Tempy = Dots[i].y - Center.y;
		DotsBuf[i*2] = Tempx*fcos(Angle) - Tempy*fsin(-Angle) + Center.x;
		DotsBuf[i*2 + 1] = Tempx*fsin(-Angle) + Tempy*fcos(Angle) + Center.y;
	}
	return IsAlive();
}

struct MovableStar
{
	float vector = random(0.0, 2.0*pi);
	float speed = random(1.0, 25.0);
	float x = ScreenHalfWidth + fcos(vector) * speed * 10.0f;
	float y = ScreenHalfHeight + fsin(vector) * speed * 10.0f;

	bool inScreen(int w, int h){return (x > 0 && y > 0 && x < w && y < h);}
};

struct StaticStar
{
	double x = random((ScreenWidth - Diagonal)/2.0 - 25, Diagonal + 25);
	double y = random((ScreenWidth - Diagonal)/2.0 - 25, Diagonal + 25);

	int Color = COLOR(random(160, 255), random(160, 255), random(160, 255));
};

int main()
{
	srand(time(0));
	initwindow(ScreenWidth, ScreenHeight, "Space War", 100, 50, true, false);
	#ifdef IncludeCosTable
	InitCosTable();
	#endif
	const int StarsCount = 1750;
	int GameProccessed = GameEnded, Kills, PlayerDots[8], Mousex, Mousey, PlayerMove, iddqd, idkfa, LoseDelay, PlayerBlowUp;
	float Tempx, Tempy;
	double PlayingTime, k, EnergyGraph[30];
	Ship Player;
	EnemyList<Bull> Bulls(7);
	EnemyList<Turret> Turrets(4);
	EnemyList<LaserWall> Lasers(2);
	BulletsArray PlayerBullets(192, 255, 255, 8, 2), EnemyBullets(255, 128, 128, 6, 4), LaserBullets(255, 64, 64, 18, 20);
	bool Shooting, Lose;
	while(menuProcess(GameProccessed))
	{
		StaticStar Stars[StarsCount];
		GameProccessed = GameInProcess;
		PlayingTime = 0.0;
		Kills = 0;
		k = random(1.0, 3.0);
		Player.Reset();
		PlayerBullets.DeleteAll();
		EnemyBullets.DeleteAll();
		LaserBullets.DeleteAll();
		Bulls.DeleteAll();
		Turrets.DeleteAll();
		Lasers.DeleteAll();
		PlayerMove = Ship::Ahead;
		Mousex = ScreenHalfWidth;
		Mousey = 0;
		Shooting = false;
		Lose = false;
		ClearInput();
		iddqd = 0;
		idkfa = 0;
		LoseDelay = 200;
		PlayerBlowUp = 0;
		for(int i = 0; i < 30; i++) EnergyGraph[i] = 100.0;
		while(GameProccessed)
		{
			cleardevice();
			for(int i = 0; i < StarsCount; i++)
			{
				Tempx = Stars[i].x - ScreenHalfWidth;
				Tempy = Stars[i].y - ScreenHalfHeight;
				Stars[i].x = Tempx * cos0_0005 - Tempy * sin0_0005;
				Stars[i].y = Tempx * sin0_0005 + Tempy * cos0_0005;
				Stars[i].x += ScreenHalfWidth;
				Stars[i].y += ScreenHalfHeight;
				putpixel(Stars[i].x - 25.0*Player.GetCenter(Ship::Center_x)/ScreenWidth, Stars[i].y - 25.0*Player.GetCenter(Ship::Center_y)/ScreenHeight, Stars[i].Color);
			}

			Bulls.DrawEnemys();
			Turrets.DrawEnemys();
			Lasers.DrawEnemys();

			if(Player.GetDots(PlayerDots))
			{
				setfillstyle(SOLID_FILL, COLOR(0, 128, 0));
				setcolor(COLOR(0, 254, 0));
				fillpoly(4, PlayerDots);
			}
			PlayerBullets.DrawBullets();
			EnemyBullets.DrawBullets();
			LaserBullets.DrawBullets();

			DrawGui(Player.GetHealth(), PlayingTime, Player.GetEnergy(), EnergyGraph, Kills, k, Lose);

			if(Lose)
			{
				if(LoseDelay)
				{
					Bulls.ProcessEnemys<double, double, bool, Ship & >(Player.GetCenter(Ship::Center_x), Player.GetCenter(Ship::Center_y), Player.IsAlive(), Player);
					Bulls.CheckForHits(PlayerBullets);
					Kills += Bulls.CheckForDead();

					Turrets.ProcessEnemys<double, double, bool, BulletsArray & >(Player.GetCenter(Ship::Center_x), Player.GetCenter(Ship::Center_y), Player.IsAlive(), EnemyBullets);
					Turrets.CheckForHits(PlayerBullets);
					Kills += Turrets.CheckForDead();

					Lasers.ProcessEnemys<double, double, bool, BulletsArray & >(Player.GetCenter(Ship::Center_x), Player.GetCenter(Ship::Center_y), Player.IsAlive(), LaserBullets);
					Lasers.CheckForHits(PlayerBullets);
					Kills += Lasers.CheckForDead();

					PlayerBullets.MoveBullets();
					PlayerBullets.CheckForDeletion();
					EnemyBullets.MoveBullets();
					EnemyBullets.CheckForDeletion();
					LaserBullets.MoveBullets();
					LaserBullets.CheckForDeletion();

					if(PlayerBlowUp < 15)
					{
						double xy[2];
						Player.GetCenter(xy);
						setcolor(COLOR(255, 64, 0));
						setfillstyle(SOLID_FILL, COLOR(255, 128, 0));
						fillellipse(xy[0], xy[1], PlayerBlowUp*3, PlayerBlowUp*3);
						PlayerBlowUp++;
					}

					LoseDelay--;
					swapbuffers();
					delay(DelayTime);
					continue;
				}
				else
				{
					GameProccessed = loseProcess();
					if(GameProccessed == GameRestarting)
						break;
					swapbuffers();
					delay(DelayTime);
					continue;
				}
			}

			if(GameProccessed == GamePaused)
			{
				GameProccessed = pauseProcess();
				if(GameProccessed == GameRestarting)
					break;
				swapbuffers();
				delay(DelayTime);
				continue;
			}

			if(kbhit())
				switch(getch())
				{
				case 27:
					ClearInput();
					Shooting = false;
					GameProccessed = pauseProcess();
					break;

				case 'W':
				case 'w':
					Player.SetAcceleration(Ship::SpeedUp);
					PlayerMove = Ship::Ahead;
					break;

				case 'D':
				case 'd':
					Player.SetAcceleration(Ship::SpeedUp);
					PlayerMove = Ship::Right;
					if(iddqd == 1 || iddqd == 2 || iddqd == 4)
						iddqd++;
					else
						iddqd = 0;
					if(idkfa == 1)
						idkfa++;
					else
						idkfa = 0;
					break;

				case 'A':
				case 'a':
					Player.SetAcceleration(Ship::SpeedUp);
					PlayerMove = Ship::Left;
					if(idkfa == 4)
						idkfa++;
					else
						idkfa = 0;
					break;

				case 'I':
				case 'i':
					if(iddqd == 0)
						iddqd++;
					if(idkfa == 0)
						idkfa++;
					break;

				case 'Q':
				case 'q':
					if(iddqd == 3)
						iddqd++;
					else
						iddqd = 0;
					break;

				case 'K':
				case 'k':
					if(idkfa == 2)
						idkfa++;
					else
						idkfa = 0;
					break;

				case 'F':
				case 'f':
					if(idkfa == 3)
						idkfa++;
					else
						idkfa = 0;
					break;
				}
			if(ismouseclick(WM_MOUSEMOVE))
				getmouseclick(WM_MOUSEMOVE, Mousex, Mousey);
			if(ismouseclick(WM_LBUTTONDOWN))
			{
				clearmouseclick(WM_LBUTTONDOWN);
				Shooting = true;
			}
			if(ismouseclick(WM_LBUTTONUP))
			{
				clearmouseclick(WM_LBUTTONUP);
				Shooting = false;
			}

			Player.SetAngle(Mousex, Mousey);
			Player.MoveShip(PlayerMove);
			Player.CalcAcceleration();
			Player.SetAcceleration(Ship::SpeedDown);
			if(Shooting)
				Player.Shoot(PlayerBullets);

			Bulls.ProcessEnemys<double, double, bool, Ship &>(Player.GetCenter(Ship::Center_x), Player.GetCenter(Ship::Center_y), Player.IsAlive(), Player);
			Kills += Bulls.CheckForDead();
			if(SpawnChance(5.0, 5.0, PlayingTime, Bulls.GetEnemysCount()))
				Bulls.SpawnEnemy();

			Turrets.ProcessEnemys<double, double, bool, BulletsArray &>(Player.GetCenter(Ship::Center_x), Player.GetCenter(Ship::Center_y), Player.IsAlive(), EnemyBullets);
			Kills += Turrets.CheckForDead();
			if(SpawnChance(2.5, 2.5, PlayingTime, Turrets.GetEnemysCount() + 1))
				Turrets.SpawnEnemy();

			Lasers.ProcessEnemys<double, double, bool, BulletsArray &>(Player.GetCenter(Ship::Center_x), Player.GetCenter(Ship::Center_y), Player.IsAlive(), LaserBullets);
			Kills += Lasers.CheckForDead();
			if(SpawnChance(1.0, 1.0, PlayingTime, Lasers.GetEnemysCount() + 1))
				Lasers.SpawnEnemy();


			if(!Player.IsAlive())
			{
				Lose = true;
				continue;
			}

			Player.EnergyRegenerate();
			Player.HealthRegenerate();
			Player.RefreshCoolDown();

			PlayerBullets.MoveBullets();
			PlayerBullets.CheckForDeletion();
			EnemyBullets.MoveBullets();
			EnemyBullets.CheckForDeletion();
			LaserBullets.MoveBullets();
			LaserBullets.CheckForDeletion();

			Bulls.CheckForHits(PlayerBullets);
			Turrets.CheckForHits(PlayerBullets);
			Lasers.CheckForHits(PlayerBullets);
			Player.CheckForHits(EnemyBullets);
			Player.CheckForHits(LaserBullets, 50.0);

			if(iddqd == 5)
			{
				iddqd = 0;
				Player.SetGodMode();
			}
			if(idkfa == 5)
			{
				idkfa = 0;
				Player.SetInfinityEnergy();
			}
			PlayingTime += TimeCounter;

			swapbuffers();
			delay(DelayTime);
		}
		ClearInput();
	}
	#ifdef IncludeCosTable
	ClearCosTable();
	#endif
	closegraph();
	return 0;
}

static bool menuProcess(int EndGameState)
{
	if(EndGameState == GameRestarting)
		return true;
	bool PlayClicked = false, QuitClicked = false;
	const int StarsCount = 750;
	MovableStar Stars[StarsCount];
	int x, y;
	while(1)
	{
		cleardevice();

		setcolor(COLOR(0, 255, 0));
		settextstyle(DEFAULT_FONT, HORIZ_DIR, 10);
		outtextxy((ScreenWidth - textwidth(const_cast<char *>("SPACE WAR")))/2, ScreenHeight/5, const_cast<char *>("SPACE WAR"));

		settextstyle(DEFAULT_FONT, HORIZ_DIR, 4);

		if(ismouseclick(WM_MOUSEMOVE))
			getmouseclick(WM_MOUSEMOVE, x, y);

		if(!QuitClicked && (ScreenWidth - textwidth(sPlay))/2 < x && ScreenHeight*2/5 < y && (ScreenWidth + textwidth(sPlay))/2 > x && ScreenHeight*2/5 + textheight(sPlay) > y)
		{
			setcolor(COLOR(0, 255, 0));
			if(ismouseclick(WM_LBUTTONDOWN))
			{
				clearmouseclick(WM_LBUTTONDOWN);
				PlayClicked = true;
			}
			if(PlayClicked && ismouseclick(WM_LBUTTONUP))
			{
				clearmouseclick(WM_LBUTTONUP);
				return true;
			}
		}
		else
			setcolor(WHITE);
		if(PlayClicked && !ismouseclick(WM_LBUTTONUP))
			setcolor(COLOR(255, 0, 0));
		else
			PlayClicked = false;
		outtextxy((ScreenWidth - textwidth(sPlay))/2, ScreenHeight*2/5, sPlay);

		if(!PlayClicked && (ScreenWidth - textwidth(sQuit))/2 < x && ScreenHalfHeight < y && (ScreenWidth + textwidth(sQuit))/2 > x && ScreenHalfHeight + textheight(sQuit) > y)
		{
			setcolor(COLOR(0, 255, 0));
			if(ismouseclick(WM_LBUTTONDOWN))
			{
				clearmouseclick(WM_LBUTTONDOWN);
				QuitClicked = true;
			}
			if(QuitClicked && ismouseclick(WM_LBUTTONUP))
			{
				clearmouseclick(WM_LBUTTONUP);
				return false;
			}
		}
		else
			setcolor(WHITE);
		if(QuitClicked && !ismouseclick(WM_LBUTTONUP))
			setcolor(COLOR(255, 0, 0));
		else
			QuitClicked = false;
		outtextxy((ScreenWidth - textwidth(sQuit))/2, ScreenHalfHeight, sQuit);

		clearmouseclick(WM_LBUTTONUP);

		setcolor(WHITE);
		for(int i = 0; i < StarsCount; i++)
			if(Stars[i].inScreen(ScreenWidth, ScreenHeight))
			{
				moveto(Stars[i].x, Stars[i].y);
				Stars[i].x += fcos(Stars[i].vector) * Stars[i].speed;
				Stars[i].y += fsin(Stars[i].vector) * Stars[i].speed;
				lineto(Stars[i].x, Stars[i].y);
			}
			else
				Stars[i] = MovableStar();

		swapbuffers();
		delay(DelayTime);
	}
}

static int pauseProcess()
{
	static bool ResumeClicked = false, ExitClicked = false, RestartClicked = false;
	static int x, y;

	setcolor(COLOR(0, 255, 0));
	rectangle(ScreenWidth/4, ScreenHeight/3, ScreenWidth*3/4, ScreenHeight*2/3);
	setfillstyle(SOLID_FILL, BLACK);
	floodfill(ScreenHalfWidth, ScreenHalfHeight, COLOR(0, 255, 0));

	settextstyle(DEFAULT_FONT, HORIZ_DIR, 4);

	outtextxy(ScreenHalfWidth - textwidth(sGamePaused)/2, ScreenHeight/3 - textheight(sGamePaused)/2, sGamePaused);

	if(ismouseclick(WM_MOUSEMOVE))
		getmouseclick(WM_MOUSEMOVE, x, y);

	if(!ExitClicked &&  !RestartClicked && (ScreenHalfWidth - textwidth(sResume)/2) < x && SHHx075 < y && (ScreenHalfWidth + textwidth(sResume)/2) > x && SHHx075 + textheight(sResume) > y)
	{
		setcolor(COLOR(0, 255, 0));
		if(ismouseclick(WM_LBUTTONDOWN))
		{
			clearmouseclick(WM_LBUTTONDOWN);
			ResumeClicked = true;
		}
		if(ResumeClicked && ismouseclick(WM_LBUTTONUP))
		{
			clearmouseclick(WM_LBUTTONUP);
			ResumeClicked = ExitClicked = RestartClicked = false;
			return GameInProcess;
		}
	}
	else
		setcolor(WHITE);
	if(ResumeClicked && !ismouseclick(WM_LBUTTONUP))
		setcolor(COLOR(255, 0, 0));
	else
		ResumeClicked = false;
	outtextxy(ScreenHalfWidth - textwidth(sResume)/2, SHHx075, sResume);

	if(!ResumeClicked && !ExitClicked && (ScreenHalfWidth - textwidth(sRestart)/2) < x && (SHHx075 + ScreenHeight/10) < y && (ScreenHalfWidth + textwidth(sRestart)/2) > x && (SHHx075 + ScreenHeight/10) + textheight(sRestart) > y)
	{
		setcolor(COLOR(0, 255, 0));
		if(ismouseclick(WM_LBUTTONDOWN))
		{
			clearmouseclick(WM_LBUTTONDOWN);
			RestartClicked = true;
		}
		if(RestartClicked && ismouseclick(WM_LBUTTONUP))
		{
			clearmouseclick(WM_LBUTTONUP);
			ResumeClicked = ExitClicked = RestartClicked = false;
			return GameRestarting;
		}
	}
	else
		setcolor(WHITE);
	if(RestartClicked && !ismouseclick(WM_LBUTTONUP))
		setcolor(COLOR(255, 0, 0));
	else
		RestartClicked = false;
	outtextxy(ScreenHalfWidth - textwidth(sRestart)/2, SHHx075 + ScreenHeight/10, sRestart);

	if(!ResumeClicked && !RestartClicked && (ScreenHalfWidth - textwidth(sExit)/2) < x && (SHHx075 + ScreenHeight/5) < y && (ScreenHalfWidth + textwidth(sExit)/2) > x &&  (SHHx075 + ScreenHeight/5) + textheight(sExit) > y)
	{
		setcolor(COLOR(0, 255, 0));
		if(ismouseclick(WM_LBUTTONDOWN))
		{
			clearmouseclick(WM_LBUTTONDOWN);
			ExitClicked = true;
		}
		if(ExitClicked && ismouseclick(WM_LBUTTONUP))
		{
			clearmouseclick(WM_LBUTTONUP);
			ResumeClicked = ExitClicked = RestartClicked = false;
			return GameEnded;
		}
	}
	else
		setcolor(WHITE);
	if(ExitClicked && !ismouseclick(WM_LBUTTONUP))
		setcolor(COLOR(255, 0, 0));
	else
		ExitClicked = false;
	outtextxy(ScreenHalfWidth - textwidth(sExit)/2, SHHx075 + ScreenHeight/5, sExit);

	clearmouseclick(WM_LBUTTONUP);

	if(kbhit() && getch() == 27)
		return GameInProcess;

	return GamePaused;
}

static int loseProcess()
{
	static bool RestartClicked = false, ExitClicked = false;
	static int x, y;

	setcolor(COLOR(254, 0, 0));
	rectangle(ScreenWidth/4, ScreenHalfHeight*3/4, ScreenWidth*3/4, ScreenHalfHeight*5/4);
	setfillstyle(SOLID_FILL, BLACK);
	floodfill(ScreenHalfWidth, ScreenHalfHeight, COLOR(254, 0, 0));

	settextstyle(DEFAULT_FONT, HORIZ_DIR, 4);

	outtextxy(ScreenHalfWidth - textwidth(sYouLose)/2, ScreenHalfHeight*3/4 - textheight(sYouLose)/2, sYouLose);

	if(ismouseclick(WM_MOUSEMOVE))
		getmouseclick(WM_MOUSEMOVE, x, y);

	if(!ExitClicked && (ScreenHalfWidth - textwidth(sRestart)/2) < x && (ScreenHalfHeight*7/8 - textheight(sRestart)/2) < y && (ScreenHalfWidth + textwidth(sRestart)/2) > x && ScreenHalfHeight*7/8 + textheight(sRestart)/2 > y)
	{
		setcolor(COLOR(255, 0, 0));
		if(ismouseclick(WM_LBUTTONDOWN))
		{
			clearmouseclick(WM_LBUTTONDOWN);
			RestartClicked = true;
		}
		if(RestartClicked && ismouseclick(WM_LBUTTONUP))
		{
			clearmouseclick(WM_LBUTTONUP);
			RestartClicked = ExitClicked = false;
			return GameRestarting;
		}
	}
	else
		setcolor(WHITE);
	if(RestartClicked && !ismouseclick(WM_LBUTTONUP))
		setcolor(COLOR(0, 255, 0));
	else
		RestartClicked = false;
	outtextxy(ScreenHalfWidth - textwidth(sRestart)/2, ScreenHalfHeight*7/8 - textheight(sRestart)/2, sRestart);

	if(!RestartClicked && (ScreenHalfWidth - textwidth(sExit)/2) < x && (ScreenHalfHeight*9/8 - textheight(sExit)/2) < y && (ScreenHalfWidth + textwidth(sExit)/2) > x && ScreenHalfHeight*9/8 + textheight(sExit) > y)
	{
		setcolor(COLOR(255, 0, 0));
		if(ismouseclick(WM_LBUTTONDOWN))
		{
			clearmouseclick(WM_LBUTTONDOWN);
			ExitClicked = true;
		}
		if(ExitClicked && ismouseclick(WM_LBUTTONUP))
		{
			clearmouseclick(WM_LBUTTONUP);
			RestartClicked = ExitClicked = false;
			return GameEnded;
		}
	}
	else
		setcolor(WHITE);
	if(ExitClicked && !ismouseclick(WM_LBUTTONUP))
		setcolor(COLOR(0, 255, 0));
	else
		ExitClicked = false;
	outtextxy(ScreenHalfWidth - textwidth(sExit)/2, ScreenHalfHeight*9/8 - textheight(sExit)/2, sExit);

	clearmouseclick(WM_LBUTTONUP);

	return LoseProcessed;
}

static void DrawGui(double Health, double Time, int Energy, double EnergyGraph[30], unsigned int Kills, double k, bool LoseBulb)
{
	static bool GodModeUsed, InfEnergyUsed;
	static counter<2> EnergyGraphCounter;

	if(Kills == 0)
		GodModeUsed = InfEnergyUsed = false;

	setcolor(COLOR(0, 255, 0));
	setfillstyle(SOLID_FILL, BLACK);

	moveto(0, 50);
	lineto(175, 50);
	lineto(225, 0);

	moveto(ScreenWidth, 50);
	lineto(ScreenWidth - 175, 50);
	lineto(ScreenWidth - 225, 0);

	moveto(0, ScreenHeight - 75);
	lineto(75, ScreenHeight - 75);
	lineto(125, ScreenHeight);
	rectangle(70, ScreenHeight - 30, 100, ScreenHeight - 10);

	moveto(ScreenWidth, ScreenHeight - 75);
	lineto(ScreenWidth - 75, ScreenHeight - 75);
	lineto(ScreenWidth - 125, ScreenHeight);
	rectangle(ScreenWidth - 68, ScreenHeight - 68, ScreenWidth - 7, ScreenHeight - 7);

	line(92, ScreenHeight - 50, ScreenWidth - 92, ScreenHeight - 50);


	setfillstyle(SOLID_FILL, BLACK);
	floodfill(0, 0, COLOR(0, 255, 0));
	floodfill(ScreenWidth - 1, 0, COLOR(0, 255, 0));
	floodfill(ScreenWidth - 11, ScreenHeight - 11, COLOR(0, 255, 0));
	floodfill(85, ScreenHeight - 20, COLOR(0, 255, 0));
	setfillstyle(SOLID_FILL, COLOR(0, 32, 0));
	floodfill(0, ScreenHeight - 1, COLOR(0, 255, 0));
	floodfill(ScreenWidth - 1, ScreenHeight - 1, COLOR(0, 255, 0));
	floodfill(ScreenHalfWidth, ScreenHeight - 1, COLOR(0, 255, 0));

	setfillstyle(SOLID_FILL, BLACK);
	bar(150, ScreenHeight - 40, ScreenWidth - 150, ScreenHeight - 10);
	if(Health < 0.0)
	{
		setfillstyle(SOLID_FILL, COLOR(160, 192, 224));
		Health = 100.0;
		GodModeUsed = true;
	}
	else
		setfillstyle(SOLID_FILL, COLOR(255 * min((100.0 - Health)/50.0, 1.0), 255 * min(Health/50.0, 1.0), 0));
	bar(150, ScreenHeight - 40, (ScreenWidth - 300)*(Health/100.0) + 150, ScreenHeight - 10);

	setcolor(COLOR(0, 255, 0));
	settextstyle(SANS_SERIF_FONT, HORIZ_DIR, 5);
	char buf[15];
	ConvertTime(Time, buf, 2);
	outtextxy(175 - textwidth(buf), 25 - textheight(buf)/2, buf);

	numberToString(Kills, buf);
	outtextxy(ScreenWidth - 175, 25 - textheight(buf)/2, buf);

	setcolor(COLOR(0, 128, 0));
	line(ScreenWidth - 67, ScreenHeight - 37, ScreenWidth - 8, ScreenHeight - 37);
	line(ScreenWidth - 37, ScreenHeight - 67, ScreenWidth - 37, ScreenHeight - 8);
	setcolor(COLOR(0, 255, 0));
	float s;
	int i;
	static double Graph = 0.0;
	moveto(ScreenWidth - 67, ScreenHeight - 37 + fsin(Graph - 1.0)*(Health/5.0)/2.75);
	for(i = ScreenWidth - 68, s = 0.0; i <= ScreenWidth - 8; i++, s += k*pi, Graph += 0.001)
		lineto(i, ScreenHeight - 37 + fsin(Graph + s - fcos(s))*(Health/5.0)/(fcos(s/k) + 1.75));
	setfillstyle(SOLID_FILL, GodModeUsed? COLOR(255, 0, 0): COLOR(0, 255, 0));
	setcolor(GodModeUsed? COLOR(128, 0, 0): COLOR(0, 128, 0));
	fillellipse(ScreenWidth - 80, ScreenHeight - 10, 3, 3);
	setfillstyle(SOLID_FILL, InfEnergyUsed? COLOR(255, 0, 0): COLOR(0, 255, 0));
	setcolor(InfEnergyUsed? COLOR(128, 0, 0): COLOR(0, 128, 0));
	fillellipse(ScreenWidth - 90, ScreenHeight - 10, 3, 3);
	setfillstyle(SOLID_FILL, LoseBulb? COLOR(255, 0, 0): COLOR(0, 255, 0));
	setcolor(LoseBulb? COLOR(128, 0, 0): COLOR(0, 128, 0));
	fillellipse(ScreenWidth - 80, ScreenHeight - 20, 3, 3);

	setlinestyle(SOLID_LINE, 0, 5);
	setcolor(BLACK);
	arc(40, ScreenHeight - 37, 0, 360, 25);
	if(Energy < 0.0)
	{
		setcolor(COLOR(160, 192, 224));
		Energy = 100.0;
		InfEnergyUsed = true;
	}
	else
		setcolor(COLOR(255 * min((100.0 - Energy)/50.0, 1.0), 255 * min(Energy/50.0, 1.0), 0));
	arc(40, ScreenHeight - 37, 0, 360 * Energy/100, 25);
	if(Energy < 25)
	{
		setcolor(COLOR(255, 0, 0));
		line(40, ScreenHeight - 50, 40, ScreenHeight - 36);
		line(40, ScreenHeight - 26, 40, ScreenHeight - 25);
	}
	setlinestyle(SOLID_LINE, 0, 1);
	for(i = 71; i < 100; i++)
	{
		setcolor(COLOR(192 * min((100.0 - EnergyGraph[i - 71])/50.0, 1.0), 192 * min(EnergyGraph[i - 71]/50.0, 1.0), 0));
		line(i, ScreenHeight - 11, i, ScreenHeight - 11 - 18 * EnergyGraph[i - 71]/100);
	}
	EnergyGraphCounter++;
	setcolor(COLOR(0, 128, 0));
	line(71, ScreenHeight - 25, 99, ScreenHeight - 25);
	line(71, ScreenHeight - 20, 99, ScreenHeight - 20);
	line(71, ScreenHeight - 15, 99, ScreenHeight - 15);
	line(76, ScreenHeight - 29, 76, ScreenHeight - 11);
	line(82, ScreenHeight - 29, 82, ScreenHeight - 11);
	line(88, ScreenHeight - 29, 88, ScreenHeight - 11);
	line(94, ScreenHeight - 29, 94, ScreenHeight - 11);
	if(EnergyGraphCounter)
	{
		for(i = 0; i < 29; i++)
			EnergyGraph[i] = EnergyGraph[i+1];
		EnergyGraph[28] = Energy;
	}
}

static void ClearInput()
{
	while(kbhit())
		getch();
	while(ismouseclick(WM_LBUTTONDOWN) || ismouseclick(WM_LBUTTONUP))
		clearmouseclick(WM_LBUTTONDOWN), clearmouseclick(WM_LBUTTONUP);
}

static char * numberToString(unsigned int Num, char * Str)
{
	int NumCpy = Num, Numl = 1;
	while(NumCpy /= 10)
		Numl++;
	Str[Numl--] = '\0';
	do
		Str[Numl--] = Num % 10 + 48;
	while(Num /= 10);
	return Str;
}

static char * numberToString(double Num, char * Str, unsigned int precision)
{
	double buf = Num * pow(10.0, precision);
	int IntNum = buf, Numl = 2;
	if(Num < 1.0)
		Numl += precision;
	else
		while(IntNum /= 10)
			Numl++;
	IntNum = buf;
	Str[Numl--] = '\0';
	while(precision--)
	{
		Str[Numl--] = IntNum % 10 + 48;
		IntNum /= 10;
	}
	Str[Numl--] = '.';
	do
		Str[Numl--] = IntNum % 10 + 48;
	while(IntNum /= 10);
	return Str;
}

static char * ConvertTime(double Num, char * Str, unsigned int precision)
{
	unsigned int Hourse, Minutes, i = 0;
	Hourse = static_cast<unsigned int>(Num / 3600.0);
	Num -= Hourse * 3600.0;
	Minutes = static_cast<unsigned int>(Num / 60.0);
	Num -= Minutes * 60.0;
	if(Hourse)
	{
		numberToString(Hourse, Str);
		i += strlen(Str);
		Str[i++] = ':';
		if(Minutes < 10.0)
			Str[i++] = '0';
	}
	if(Hourse || Minutes)
	{
		numberToString(Minutes, Str + i);
		i += strlen(Str + i);
		Str[i++] = ':';
		if(Num < 10.0)
			Str[i++] = '0';
	}
	if(Hourse || Minutes)
		numberToString(static_cast<unsigned int>(Num), Str + i);
	else
		numberToString(Num, Str + i, precision);
	return Str;
}
