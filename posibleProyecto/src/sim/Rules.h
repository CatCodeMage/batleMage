#pragma once

namespace sim {

// Todos los numeros que definen el combate, en un solo sitio.
// Ajustar el equilibrio del juego (o entrenar con otras reglas) no obliga a tocar la logica.
struct Rules {
    float arenaRadius = 9.0f;
    float fighterRadius = 0.7f;

    float maxHp = 100.0f;
    float maxMana = 100.0f;
    float manaRegen = 14.0f;  // por segundo

    // Lanzar magia: proyectil simple (un "palo" de energia)
    float magicCost = 18.0f;
    float magicWindup = 0.35f;     // tiempo hasta que sale el proyectil
    float magicRecovery = 0.15f;   // tiempo hasta poder hacer otra cosa
    float magicCooldown = 0.9f;
    float magicSpeed = 11.0f;
    float magicRadius = 0.3f;
    float magicDamage = 18.0f;
    float magicLife = 4.0f;
    float magicSpread = 0.06f;     // imprecision de punteria (radianes)

    // Esquivar: desplazamiento lateral con invulnerabilidad
    float dodgeCost = 15.0f;
    float dodgeDuration = 0.45f;
    float dodgeSpeed = 7.5f;
    float dodgeCooldown = 1.1f;

    // Bloquear con magia: escudo que absorbe proyectiles
    float blockCost = 25.0f;
    float blockDuration = 0.7f;
    float blockCooldown = 1.4f;
    float shieldRadius = 1.25f;
    float shieldDistance = 1.1f;   // distancia del centro del escudo al combatiente
};

}  // namespace sim
