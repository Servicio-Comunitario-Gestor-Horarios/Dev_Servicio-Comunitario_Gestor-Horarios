#include "backend/services/preferencias.hpp"

QString textoDeSentido(SentidoPreferencia sentido)
{
    switch (sentido)
    {
    case SentidoPreferencia::Preferida:
        return QStringLiteral("preferida");
    case SentidoPreferencia::Evitada:
        return QStringLiteral("evitada");
    }
    return QString();
}

bool sentidoDesdeTexto(const QString& texto, SentidoPreferencia* sentido)
{
    if (texto == QStringLiteral("preferida"))
    {
        if (sentido)
            *sentido = SentidoPreferencia::Preferida;
        return true;
    }

    if (texto == QStringLiteral("evitada"))
    {
        if (sentido)
            *sentido = SentidoPreferencia::Evitada;
        return true;
    }

    return false;
}

bool sentidoValido(SentidoPreferencia sentido)
{
    return sentido == SentidoPreferencia::Preferida || sentido == SentidoPreferencia::Evitada;
}

bool haySolape(const FranjaHoraria& a, const FranjaHoraria& b)
{
    return a.inicio < b.fin && b.inicio < a.fin;
}
