#ifndef CONTROLADOR_PERSONAJE_H
#define CONTROLADOR_PERSONAJE_H

class Personaje;
class ControladorPersonaje
{

public:
    virtual ~ControladorPersonaje() = default;
    virtual void actualizar(Personaje& personaje, float deltaTime) = 0;
};

#endif
