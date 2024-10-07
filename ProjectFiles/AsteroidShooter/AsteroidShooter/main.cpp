#include <assert.h>
#include <string>
#include <vector>

#include "SFML/Graphics.hpp"
#include "Game.h"

using namespace sf;
using namespace std;


bool LoadTexture(const string& file, Texture& tex)
{
	if (tex.loadFromFile(file))
	{
		tex.setSmooth(true);
		return true;
	}
	assert(false);
	return false;
}

//for ship and asteroid objects
struct Object {
	enum class objT{Ship,Rock,Bullet}; //used to check what type of object it is 
	objT type; //stores what type of object it is
	Sprite spr;
	float dist;
	float radius = 6.5f;
	bool active = true;
	int health = 100.0f;
	sf::Text healthText;
	Font font;
	float f = 1.0f;
	int i = 0;

	void Update(const Vector2u& screenSz, float elapsed, vector <Object>& objects, IntRect& texR, RenderWindow &window, Clock& clock){
		//checks what type the object is 
		if (active) {
			switch (type) {
			case objT::Ship:
				//if the object is the ship, allow the player to control it
				playerControl(screenSz, elapsed, objects, window);
				destroy(objects);
				playerCollision(objects);
				displayShipHealth(objects);
				break;
			case objT::Rock:
				updateRock(elapsed);
				checkRockOverlap(objects);
				resetInactiveRocks(objects);
				destroy(objects);
				break;
			case objT::Bullet:
				checkBulletOverlap(objects);
				updateBullet(elapsed);
				detectBulletHit(objects);
				break;
			default:
				assert(false);
			}
		}
	}

	void playerControl(const Vector2u& screenSz, float elapsed, vector <Object>& objects, RenderWindow &window) {
		Vector2f pos = spr.getPosition(); //get current player position
		const float SPEED = 250.f; 
		//check keyboard press
		if (Keyboard::isKeyPressed(Keyboard::Up))
		{
			//if up key is pressed, pos.y is decreased by speed*elapsed time
			if (pos.y > 0)
				pos.y -= SPEED * elapsed;
		}

		if (Keyboard::isKeyPressed(Keyboard::Down))
		{
			if (pos.y < (screenSz.y - spr.getGlobalBounds().height))
				pos.y += SPEED * elapsed;
		}

		if (Keyboard::isKeyPressed(Keyboard::Left))
		{
			if (pos.x > (0 + spr.getGlobalBounds().width))
				pos.x -= SPEED * elapsed;
		}

		if (Keyboard::isKeyPressed(Keyboard::Right))
		{
			if (pos.x < screenSz.x)
				pos.x += SPEED * elapsed;
		}
		
		if (Keyboard::isKeyPressed(Keyboard::Space)) {
			shootBullet(objects, pos);

		}

		if (health <= 0) {
			window.close();
		}
		
		spr.setPosition(pos);
	}

	void updateRock(float elapsed) {
		Vector2f pos = spr.getPosition();
		float x = pos.x - GC::ROCK_SPEED * elapsed * 2;
		if (x < -spr.getGlobalBounds().width / 2) {
			active = false;
		}
		spr.setPosition(x, pos.y);
	}

	//renders object to screen
	void Render(RenderWindow &window) {
		if (active) {
			window.draw(spr);
		}
		window.draw(healthText);
	}
	//used to initialise the object differently depending on the type of object
	void initialise(RenderWindow &window, Texture& tex, objT type) {
		switch (type) {
		case objT::Ship:
			//if the object is ship, initialise ship
			InitShip(window, tex);
			break;
		case objT::Rock:
			InitRock(window, tex);
			break;
		case objT::Bullet:
			initBullet(window, tex);
			break;
		default:
			assert(false);
		}
	}
	//initialising object for it to be used
	void InitShip(RenderWindow& window, Texture& tex) {
		type = objT::Ship; //setting the type to ship
		radius = 5.f;
		spr.setTexture(tex);
		IntRect texR = spr.getTextureRect();
		spr.setScale(0.1f, 0.1f);
		spr.setOrigin(spr.getGlobalBounds().width / 2, spr.getGlobalBounds().height / 2);
		spr.setRotation(90);
		spr.setPosition(window.getSize().x * 0.05f, window.getSize().y / 2.f);
		initHealthText();
	}
	//initialising object for it to be used
	void InitRock(RenderWindow& window, Texture& tex) {
		type = objT::Rock; //setting the type to rock (asteroid)
		radius = 6.f;
		spr.setTexture(tex);
		spr.setScale(0.5f, 0.5f);
		IntRect texR(0, 0, 96, 96);
		spr.setTextureRect(texR);
		spr.setOrigin(spr.getTextureRect().width / 2, spr.getTextureRect().height / 2);
		spr.setPosition(window.getSize().x - spr.getGlobalBounds().width, window.getSize().y / 2);
	}

	void placeRandRocks(vector <Object>& objects, Object rock) {
		objects.insert(objects.end(), 30, rock);
		for (int i = 0; i < 30; i++) {

			int x = rand() % GC::SCREEN_RES.x * 2 + GC::SCREEN_RES.x;
			int y = rand() % GC::SCREEN_RES.y + 1;

			if (objects.at(i).type == Object::objT::Rock) {
				objects.at(i).spr.setPosition(x, y);

				float ranScale = rand() % 80 + 35;
				objects.at(i).spr.setScale(ranScale / 100, ranScale / 100);
				objects.at(i).radius = objects.at(i).radius * ((ranScale) / 100);
			}
		}
	}

	bool circleToCircle(const Vector2f pos1, const Vector2f pos2) {
		float dist = (pos1.x - pos2.x) * (pos1.x - pos2.x) + (pos1.y - pos2.y) * (pos1.y - pos2.y);
		dist = sqrtf(dist);
		if (dist - 10 <= radius * radius) {
			return true;
		}
		else {
			return false;
		}
	}

	void checkRockOverlap(vector <Object>& objects) {
		for (int i = 0; i < objects.size() - 1; i++) {
			if (objects.at(i).type == Object::objT::Rock) {
				for (int j = 0; j < objects.size() - 1; j++) {
					if (j == i) {
						if (i + 1 <= objects.size()) {
							j = i + 1;
						}
					}
					if (objects.at(j).type != Object::objT::Ship) {
						if (objects.at(j).circleToCircle(objects.at(i).spr.getPosition(), objects.at(j).spr.getPosition())) {
							objects.at(j).active = false;
						}
					}
				}
			}
		}
	}

	void resetInactiveRocks(vector <Object>& objects) {
		for (int i = 0; i < objects.size() - 1; i++) {
			if (objects.at(i).type == Object::objT::Rock) {
				if (objects.at(i).active == false) {
					objects.at(i).spr.setPosition(GC::SCREEN_RES.x + objects.at(i).spr.getGlobalBounds().width, rand() % GC::SCREEN_RES.y + 1/*objects.at(i).spr.getGlobalBounds().height*/);
					objects.at(i).active = true;
				}
			}
		}
	}

	Vector2f decaySpeed(Vector2f& currentVal, float pcnt, float timeInterval, float dTime) {
		float mod = 1.0f - pcnt * (dTime / timeInterval);
		Vector2f alpha(currentVal.x * mod, currentVal.y * mod);
		return alpha;
	}

	void initBullet(RenderWindow &window, Texture& tex) {
		type = objT::Bullet;
		spr.setTexture(tex);
		IntRect texR(0, 0, 32, 32);
		spr.setTextureRect(texR);
		spr.setOrigin(texR.width / 2, texR.height / 2);
		radius = 5.0f;
		float scale = 1.f;
		spr.setScale(scale, scale);
		active = false;
	}

	void shootBullet(vector <Object>& objects, Vector2f currentPos) {
		bool found = false;
		for (int i = 0; i < objects.size(); i++) {
			if (objects.at(i).active == false && objects.at(i).type == Object::objT::Bullet && found == false) {
				objects.at(i).active = true;
				objects.at(i).spr.setPosition(currentPos.x,currentPos.y);
				found = true;
			}
		}
		found = false;
	}

	void detectBulletHit(vector <Object>& objects) {
		for (int i = 0; i < objects.size(); i++) {
			if (objects.at(i).type == Object::objT::Bullet && objects.at(i).active == true) {
				for (int j = 0; j < objects.size(); j++) {
					if (objects.at(j).type == Object::objT::Rock) {
						if (circleToCircle(objects.at(i).spr.getPosition(), objects.at(j).spr.getPosition())) {
							objects.at(j).health -= 50;
							objects.at(i).active = false;
						}
					}
				}
			}
		}
	}

	void updateBullet(float elapsed) {
		Vector2f pos = spr.getPosition();
		pos.x += elapsed * GC::BULLET_SPEED;
		if (active) {
			spr.setPosition(pos);
		}
	}

	void destroy(vector <Object>& objects) {
		if (health <= 0) {
			active = false;
			health = 100;
		}
	}

	void checkBulletOverlap(vector <Object>& objects) {
		for (int i = 0; i < objects.size(); i++) {
			if (objects.at(i).type == Object::objT::Bullet && objects.at(i).active == true) {
				for (int j = 0; j < objects.size(); j++) {
					if (j == i) {
						j = i + 1;
					}
					if (objects.at(j).type == Object::objT::Bullet && objects.at(j).active == true) {
						if (circleToCircle(objects.at(i).spr.getPosition(), objects.at(j).spr.getPosition())) {
							objects.at(j).active = false;
						}
					}
				}
			}
		}
	}

	void playerCollision(vector <Object>& objects) {
		for (int i = 0; i < objects.size(); i++) {
			if (objects.at(i).type == Object::objT::Ship) {
				for (int j = 0; j < objects.size(); j++) {
					if (objects.at(j).type == Object::objT::Rock && objects.at(i).active == true) {
						if (circleToCircle(objects.at(i).spr.getPosition(), objects.at(j).spr.getPosition())) {
							objects.at(j).active = false;
							objects.at(i).health -= 50;
						}
					}
				}
			}
		}
	}

	void displayShipHealth(vector <Object>& objects) {
		for (int i = 0; i < objects.size(); i++) {
			if (objects.at(i).type == Object::objT::Ship) {
				if (objects.at(i).health < 100) {
					healthText.setColor(Color::Red);
				}
				healthText.setString(to_string(objects.at(i).health));
			}
		}
	}

	void initHealthText() {
		if (!font.loadFromFile("data/fonts/Roboto-BoldItalic.ttf")) {
			assert(false);
		}
		healthText.setFont(font);
		healthText.setColor(Color::Green);
		healthText.setCharacterSize(50);
		healthText.setString(to_string(health));
		healthText.setOrigin(healthText.getGlobalBounds().width / 2, healthText.getGlobalBounds().height / 2);
		healthText.setPosition(0 + healthText.getGlobalBounds().width / 2, 0 + healthText.getGlobalBounds().height / 2);
	}

};

struct mainBG {
	//sprites and textures that can be created
	Texture mainBGTex;
	Sprite mainBG;

	Texture floorTex;
	Sprite floorSpr;

	Texture cloudTex;
	Sprite cloudSpr;

	Texture mountainTex;
	Sprite mountainSprite;

	//function to find texture to use on sprite
	void findTexture(string file, Texture& tex) {
		if (!tex.loadFromFile(file)) {
			assert(false);			
		}
		//makes the texture repeat so it can scroll infinitely
		tex.setRepeated(true);
	}

	//function to create background sprite, this is different from creating normal sprites because you cant change the position and scale
	void createBackgroundSprite(RenderWindow &window) {
		mainBG.setTexture(mainBGTex);
		mainBG.setOrigin(mainBG.getGlobalBounds().width / 2, mainBG.getGlobalBounds().height / 2);
		mainBG.setPosition(window.getSize().x / 2, window.getSize().y / 2);

		IntRect sprDim = mainBG.getTextureRect();
		float width = window.getSize().x / (float)sprDim.width;
		float height = window.getSize().y / (float)sprDim.height;
		mainBG.setScale(width, height);		
	}

	//function to create, scale and position sprite 
	void createSprite(RenderWindow &window, Sprite& spr, Texture& tex, float posX, float posY, float scaleX, float scaleY) {
		spr.setTexture(tex);

		spr.setOrigin(spr.getGlobalBounds().width / 2, spr.getGlobalBounds().height / 2);
		spr.setPosition(posX, posY);

		spr.setScale(scaleX, scaleY);
	}
	//function to scroll sprite
	void scrollSprite(Sprite& spr, float timer, float scrollSpeed) {
		IntRect scroll = spr.getTextureRect();
		scroll.left = timer * scrollSpeed;
		spr.setTextureRect(scroll);
	}
	//display sprite to screen
	void drawSprite(Sprite spr, RenderWindow &window) {
		window.draw(spr);
	}
};
/*
* How do you load a sprite and then move it around the screen?
*/
int main()
{
	srand(time(NULL));
	// Create the main window
	RenderWindow window(VideoMode(GC::SCREEN_RES.x, GC::SCREEN_RES.y), "Asteroids");

	vector <Object> objects;
	Texture shipTex;
	LoadTexture("data/ship.png", shipTex);
	Object ship;
	ship.initialise(window, shipTex,Object::objT::Ship);
	objects.push_back(ship);

	Texture rockTex;
	LoadTexture("data/asteroid.png", rockTex);
	Object rock;
	rock.initialise(window, rockTex, Object::objT::Rock);

	rock.placeRandRocks(objects, rock);
	rock.checkRockOverlap(objects);

	Texture bulletTex;
	LoadTexture("data/bullet.png", bulletTex);
	Object bullet;
	for (int i = 0; i < 30; i++) {
		bullet.initialise(window, bulletTex, Object::objT::Bullet);
		objects.push_back(bullet);
	}
	
	mainBG bg;

	//finding texture for main background and creating the background sprite using it
	bg.findTexture("data/backgroundLayers/mountains01_007.png", bg.mainBGTex);
	bg.createBackgroundSprite(window);

	//finding textures for sprites and then using them to create the sprites and position them
	bg.findTexture("data/backgroundLayers/mountains01_004.png", bg.cloudTex);
	bg.createSprite(window, bg.cloudSpr, bg.cloudTex, window.getSize().x / 2, window.getSize().y / 4, 2.5f, 2.5f);

	bg.findTexture("data/backgroundLayers/mountains01_006.png", bg.floorTex);
	bg.createSprite(window, bg.floorSpr, bg.floorTex, window.getSize().x / 2, window.getSize().y / 2 + 20, 3.f, 3.f);

	bg.findTexture("data/backgroundLayers/mountains01_002.png", bg.mountainTex);
	bg.createSprite(window, bg.mountainSprite, bg.mountainTex, window.getSize().x / 2, window.getSize().y / 2 - 10, 2.5f, 2.5f);

	Clock clock;
	IntRect yes(0, 0, 32, 32);
	float timer = 0;
	// Start the game loop 
	while (window.isOpen())
	{
		// Process events
		Event event;
		while (window.pollEvent(event))
		{
			// Close window: exit
			if (event.type == Event::Closed) 
				window.close();
			if (event.type == Event::TextEntered)
			{
				if (event.text.unicode == GC::ESCAPE_KEY)
					window.close(); 
			}
			
		} 

		// Clear screen
		window.clear();

		Vector2u screenSz = window.getSize();
		timer += clock.getElapsedTime().asSeconds();
		float elapsed = clock.getElapsedTime().asSeconds();
		clock.restart();

		bg.scrollSprite(bg.cloudSpr, timer, 20);
		bg.scrollSprite(bg.mountainSprite, timer, 30); //mountain scrolls twice as fast to create 3d effect

		//displaying background sprites on screen
		bg.drawSprite(bg.mainBG, window);
		bg.drawSprite(bg.cloudSpr, window);
		bg.drawSprite(bg.floorSpr, window);
		bg.drawSprite(bg.mountainSprite, window);

		for (int i = 0; i < objects.size(); i++) {
			objects.at(i).Update(screenSz, elapsed, objects,yes,window, clock);
			objects.at(i).Render(window);
		}


		for (int i = 0; i < objects.size(); i++) {
			if (objects.at(i).type == Object::objT::Ship) {
				for (int j = 0; j < objects.size(); j++) {
					if (j == i) {
						j = i + 1;
					}
					if (objects.at(j).active) {
						objects.at(j).circleToCircle(objects.at(i).spr.getPosition(), objects.at(j).spr.getPosition());
					}
				}
			}
		}

		// Update the window
		window.display();

		
	}

	return EXIT_SUCCESS;
}
