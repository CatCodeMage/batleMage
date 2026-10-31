#include "personaje.h"
#include <iostream>
#include <fstream>
using namespace std;

Personaje::Personaje(int id, glm::vec3 posicionInicial, float radio, HechizosGestor* gestorHechizos)
{
    this->m_id = id;
    this->m_posicion = posicionInicial;
    this->m_escala = glm::vec3(1.0f,1.0f,1.0f);
    this->m_rotacion = glm::vec3(0.0f,0.0f,0.0f);
    this->m_direccion = glm::vec3(0.0f, 0.0f, 1.0f);
    this->m_radio = radio;
    this->m_margenCasteo = 0.5f;
    this->m_vidaMax = 100.0f;
    this->m_vida = this->m_vidaMax;
    this->m_manaMax = 50.0f;
    this->m_mana = this->m_manaMax;
    this->m_tiempoQuemado = 0.0f;
    this->m_tiempoCongelado = 0.0f;
    this->m_danoQuemadura = 5.0f;
    anadirEstado(EstadoPersonaje::IDLE);
    m_controlador = nullptr;
    m_gestorHechizos = gestorHechizos;

}

void Personaje::anadirEstado(EstadoPersonaje nuevoEstado, float duracion)
{
    if(nuevoEstado == EstadoPersonaje::QUEMADO)
    {
        m_tiempoQuemado = duracion;
    }
    else if(nuevoEstado == EstadoPersonaje::CONGELADO)
    {
        m_tiempoCongelado = duracion;
    }
    if(!yaTieneEstado(nuevoEstado))
    {
        m_estadosActuales.push_back(nuevoEstado);
    }
}

void Personaje::borrarEstado(EstadoPersonaje estado)
{
    if(estado == EstadoPersonaje::QUEMADO) m_tiempoQuemado = 0.0f;
    if(estado == EstadoPersonaje::CONGELADO) m_tiempoCongelado = 0.0f;

    auto it = std::find(m_estadosActuales.begin(), m_estadosActuales.end(), estado);
    if(it != m_estadosActuales.end())
    {
        m_estadosActuales.erase(it);
    }
}

bool Personaje::yaTieneEstado(EstadoPersonaje estado) const
{
    return std::find(m_estadosActuales.begin(), m_estadosActuales.end(), estado) != m_estadosActuales.end();
}

void Personaje::update(float deltaTime)
{
    if(yaTieneEstado(EstadoPersonaje::MUERTO)) return;

    if(yaTieneEstado(EstadoPersonaje::QUEMADO))
    {
        recibirDano(m_danoQuemadura * deltaTime);
        m_tiempoQuemado -= deltaTime;
        
        if(yaTieneEstado(EstadoPersonaje::MUERTO)) return;

        if(m_tiempoQuemado <= 0.0f)
        {
            borrarEstado(EstadoPersonaje::QUEMADO);
        }
    }

    if(yaTieneEstado(EstadoPersonaje::CONGELADO))
    {
        m_tiempoCongelado -= deltaTime;
        if(m_tiempoCongelado <= 0.0f)
        {
            borrarEstado(EstadoPersonaje::CONGELADO);
        }
        else
        {
            return;
        }
    }
    
    if(m_controlador != nullptr)
    {
        m_controlador->actualizar(*this, deltaTime);
    }
}

void Personaje::moverse(const glm::vec3& desplazamiento)
{
    if (yaTieneEstado(EstadoPersonaje::MUERTO) || yaTieneEstado(EstadoPersonaje::CONGELADO)) return;

    m_posicion = m_posicion + desplazamiento;
    
    borrarEstado(EstadoPersonaje::IDLE);
    anadirEstado(EstadoPersonaje::VOLANDO);
}

void Personaje::rotar(const glm::vec3& rotacion)
{
    if (yaTieneEstado(EstadoPersonaje::MUERTO) || yaTieneEstado(EstadoPersonaje::CONGELADO)) return;

    m_rotacion  = m_rotacion + rotacion;
}

void Personaje::recibirDano(float cantidadDano)
{
    if(yaTieneEstado(EstadoPersonaje::MUERTO)) return;
    m_vida -= cantidadDano;
    cout << "Personaje " << m_id << " recibe " << cantidadDano << " de dano. Vida restante: " << m_vida << endl;

    if (m_vida <= 0)
    {
        m_vida = 0;
        m_estadosActuales.clear();
        anadirEstado(EstadoPersonaje::MUERTO);
        cout << "Personaje " << m_id << " ha muerto." << endl;
    }
}

void Personaje::aplicarEfecto(EffectType efecto, float valor, float duracion)
{
    if(yaTieneEstado(EstadoPersonaje::MUERTO)) return;
    if(efecto == EffectType::Burn)
    {
        anadirEstado(EstadoPersonaje::QUEMADO, duracion);
        m_danoQuemadura = valor;
    }
    else if(efecto == EffectType::Slow)
    {
        anadirEstado(EstadoPersonaje::CONGELADO, duracion);
    }
}

void Personaje::aumentarVida(float cantidad)
{
    m_vida += cantidad;
    if(m_vida > m_vidaMax) m_vida = m_vidaMax;
}

void Personaje::aumentarMana(float cantidad)
{
    m_mana += cantidad;
    if(m_mana > m_manaMax) m_mana = m_manaMax;
}

void Personaje::disminuirMana(float cantidad)
{
    m_mana -= cantidad;
    if(m_mana < 0) m_mana = 0;
}

void Personaje::lanzarHechizo(int idHechizo)
{
    if (yaTieneEstado(EstadoPersonaje::MUERTO) || yaTieneEstado(EstadoPersonaje::CONGELADO)) return;
    
    anadirEstado(EstadoPersonaje::CASTEANDO);
    
    //direccion normalizada donde mira el pj
    float longitud = std::sqrt(m_direccion.x * m_direccion.x + m_direccion.y * m_direccion.y + m_direccion.z * m_direccion.z);

    glm::vec3 direccionDisparo(0.0f, 0.0f, 1.0f);
    if (longitud > 0.0001f)
    {
        direccionDisparo = glm::vec3(m_direccion.x / longitud, m_direccion.y / longitud, m_direccion.z / longitud);
    }

    if (m_gestorHechizos != nullptr) {
        // El hechizo nace con su centro fuera del mago: si no, su propio radio ya lo toca y se da a si mismo
        const float radioHechizo = m_gestorHechizos->getCatalogoHechizos().at(idHechizo).radius;
        float distanciaSeparacion = m_radio + radioHechizo + m_margenCasteo;

        glm::vec3 posicionOrigenHechizo(
            m_posicion.x + direccionDisparo.x * distanciaSeparacion,
            m_posicion.y + direccionDisparo.y * distanciaSeparacion,
            m_posicion.z + direccionDisparo.z * distanciaSeparacion
        );

        if (m_gestorHechizos != nullptr)
        {
            m_gestorHechizos->spawnSpell(idHechizo, posicionOrigenHechizo, direccionDisparo);
        }
    }
}

void Personaje::setControlador(ControladorPersonaje* nuevoControlador)
 {
    m_controlador = nuevoControlador;
}

int Personaje::getId() const { return m_id; }

float Personaje::getVida() const { return m_vida; }

float Personaje::getMana() const { return m_mana; }

float Personaje::getRadio() const { return m_radio; }

glm::vec3 Personaje::getPosicion() const { return m_posicion; }
glm::vec3 Personaje::getRotacion() const { return m_rotacion; }
glm::vec3 Personaje::getDireccion() const { return m_direccion; }

void Personaje::setPosicion(const glm::vec3& nuevaPosicion) { m_posicion = nuevaPosicion; }
void Personaje::setRotacion(const glm::vec3& nuevaRotacion) { m_rotacion = nuevaRotacion; }
void Personaje::setDireccion(const glm::vec3& nuevaDireccion) { m_direccion = nuevaDireccion; }
void Personaje::setRadio(float radio) { m_radio = radio; }

std::vector<EstadoPersonaje> Personaje::getEstados() const
{
    return m_estadosActuales;
}

void Personaje::setGestorHechizos(HechizosGestor* gestor) 
{
    m_gestorHechizos = gestor;
}

HechizosGestor* Personaje::getGestorHechizos() const
{
    return m_gestorHechizos;
}
DatosRenderPersonaje Personaje::obtenerDatosParaGraficar()
{
    return DatosRenderPersonaje 
    {
        m_id,
        m_posicion,
        m_rotacion,
        m_radio,
        m_estadosActuales
    };
}

std::string Personaje::getEstadoPrincipalString() const
{
    if (yaTieneEstado(EstadoPersonaje::MUERTO))    return "dead";
    if (yaTieneEstado(EstadoPersonaje::CONGELADO)) return "frozen";
    if (yaTieneEstado(EstadoPersonaje::QUEMADO))   return "burned";
    if (yaTieneEstado(EstadoPersonaje::CASTEANDO)) return "casting";
    if (yaTieneEstado(EstadoPersonaje::VOLANDO))   return "flying";
    return "alive";
}

bool Personaje::cargarDesdeJson(const std::string& rutaArchivoPlayers, const std::string& tipoPersonaje)
{
    std::ifstream archivo(rutaArchivoPlayers);
    if (!archivo.is_open())
    {
        std::cerr << "Error: No se pudo abrir el archivo " << rutaArchivoPlayers << std::endl;
        return false;
    }

    json datosJson;
    archivo >> datosJson;

    if (!datosJson.contains(tipoPersonaje))
    {
        std::cerr << "Error: El tipo '" << tipoPersonaje << "' no existe en " << rutaArchivoPlayers << std::endl;
        return false;
    }

    m_tipoPersonaje = tipoPersonaje;
    const json& info = datosJson[tipoPersonaje];

    if (info.contains("radious"))
    {
        m_radio = info["radious"].get<float>();
    }

    if (info.contains("scale") && info["scale"].is_array() && info["scale"].size() == 3)
    {
        m_escala = glm::vec3(
            info["scale"][0].get<float>(),
            info["scale"][1].get<float>(),
            info["scale"][2].get<float>()
        );
    }

    if (info.contains("stats"))
    {
        const json& stats = info["stats"];
        if (stats.contains("maxHealth"))
        {
            m_vidaMax = stats["maxHealth"].get<float>();
            m_vida = m_vidaMax;
        }
        if (stats.contains("maxMana"))
        {
            m_manaMax = stats["maxMana"].get<float>();
            m_mana = m_manaMax;
        }
        if (stats.contains("castMargin"))
        {
            m_margenCasteo = stats["castMargin"].get<float>();
        }
        if (stats.contains("burnDamagePerSec"))
        {
            m_danoQuemadura = stats["burnDamagePerSec"].get<float>();
        }
    }

    return true;
}

json Personaje::exportarCabeceraJson() const
{
    return json{
        {"type", m_tipoPersonaje.empty() ? "sphere" : m_tipoPersonaje},
        {"position", {m_posicion.x, m_posicion.y, m_posicion.z}},
        {"scale", {m_escala.x, m_escala.y, m_escala.z}}
    };
}

json Personaje::exportarFrameJson() const
{
    return json{
        {"position", {m_posicion.x, m_posicion.y, m_posicion.z}},
        {"rotation", {m_rotacion.x, m_rotacion.y, m_rotacion.z}},
        {"state", getEstadoPrincipalString()}
    };
}