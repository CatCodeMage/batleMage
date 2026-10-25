#pragma once

#include "CombatTypes.h"

class Damage
{
private:
    float m_amount;
    ElementType m_element;
    EffectType m_effect;

    float m_effectValue;
    float m_duration;

public:
    Damage(
        float amount,
        ElementType element,
        EffectType effect = EffectType::None,
        float effectValue = 0.0f,
        float duration = 0.0f
    );

    float getAmount() const;
    ElementType getElement() const;
    EffectType getEffect() const;
    float getEffectValue() const;
    float getDuration() const;
};