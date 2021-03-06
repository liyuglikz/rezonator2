#include <QApplication>
#include <QDebug>

#include "CausticFunction.h"
#include "../funcs/PumpCalculator.h"
#include "../funcs/RoundTripCalculator.h"
#include "../core/Protocol.h"

using namespace Z;

void CausticFunction::calculate()
{
    if (!checkArgElem()) return;

    auto elem = Z::Utils::asRange(arg()->element);
    if (!elem)
    {
        setError("CausticFunction.arg.element is not range");
        return;
    }

    _wavelenSI = schema()->wavelength().value().toSi() / elem->ior();

    auto tmpRange = arg()->range;
    tmpRange.stop = Z::Value(elem->axisLengthSI(), Z::Units::m());
    auto range = tmpRange.plottingRange();
    if (!prepareResults(range)) return;
    if (!prepareCalculator(elem, true)) return;

    auto tripType = _schema->tripType();
    if (tripType == TripType::SP)
        if (!prepareSP()) return;

    BackupAndLock locker(elem, elem->paramLength());

    bool stabilityChecked = !_schema->isResonator();
    Z::PointTS prevRes(Double::nan(), Double::nan());
    for (auto x : range.values())
    {
        elem->setSubRangeSI(x);
        _calc->multMatrix();

        // After the first round-trip was calculated,
        // we should check if system is unstable
        if (!stabilityChecked)
        {
            stabilityChecked = true;
            auto stab = _calc->isStable();
            if (!stab.T && !stab.S)
            {
                setError(qApp->translate("Calc error", "System is unstable, can't calculate caustic"));
                return;
            }
        }

        Z::PointTS res;
        switch (tripType)
        {
        case TripType::SW:
        case TripType::RR: res = calculateResonator(); break;
        case TripType::SP: res = calculateSinglePass(); break;
        }

        double argX = _argumentUnit->fromSi(x);

        if (_mode == FontRadius)
        {
            // If wavefront radius changes its sign, then we have a pole at waist
            if (!std::isnan(prevRes.T) && (prevRes.T * res.T) < 0)
                _results.T.addPoint(argX, Double::nan()); // finish previous segment
            if (!std::isnan(prevRes.S) && (prevRes.S * res.S) < 0)
                _results.S.addPoint(argX, Double::nan()); // finish previous segment
            prevRes = res;
        }

        convertFromSiToModeUnits(res);
        addResultPoint(argX, res);
    }

    finishResults();
}

bool CausticFunction::prepareSP()
{    
    PumpParams *pump = _schema->activePump();
    if (!pump)
    {
        setError(qApp->translate("Calc error",
            "There is no active pump in the schema. "
            "Use 'Pumps' window to create a new pump or activate one of the existing ones."));
        return false;
    }
    if (!_pumpCalc.T) _pumpCalc.T = PumpCalculator::T();
    if (!_pumpCalc.S) _pumpCalc.S = PumpCalculator::S();
    // Cached _wavelenSI can be reduced in order to account medium IOR,
    // but pump is supposed to be in air, so get wavelength from schema.
    const double lambda = schema()->wavelength().value().toSi();
    if (!_pumpCalc.T->init(pump, lambda) ||
        !_pumpCalc.S->init(pump, lambda))
    {
        setError("Unsupported pump mode");
        return false;
    }
    return true;
}

Z::PointTS CausticFunction::calculateSinglePass() const
{
    BeamResult beamT = _pumpCalc.T->calc(_calc->Mt(), _wavelenSI);
    BeamResult beamS = _pumpCalc.S->calc(_calc->Ms(), _wavelenSI);
    switch (_mode)
    {
    case BeamRadius:
        return { beamT.beamRadius, beamS.beamRadius };

    case FontRadius:
        return { beamT.frontRadius, beamS.frontRadius };

    case HalfAngle:
        return { beamT.halfAngle, beamS.halfAngle };
    }
    qCritical() << "Unsupported caustic result mode";
    return { Double::nan(), Double::nan() };
}

Z::PointTS CausticFunction::calculateResonator() const
{
#define CALC_TS_POINT(func) return Z::PointTS(func(_calc->Mt()), func(_calc->Ms()))

    switch (_mode)
    {
    case BeamRadius: CALC_TS_POINT(calculateResonator_beamRadius);
    case FontRadius: CALC_TS_POINT(calculateResonator_frontRadius);
    case HalfAngle: CALC_TS_POINT(calculateResonator_halfAngle);
    }
    return Z::PointTS();

#undef CALC_TS_POINT
}

// TODO:NEXT-VER should be in Calculator as it can be reused by another functions, e.g. BeamParamsAtElems
double CausticFunction::calculateResonator_beamRadius(const Z::Matrix& m) const
{
    double p = 1 - SQR((m.A + m.D) * 0.5);
    if (p <= 0) return Double::nan();
    double w2 = _wavelenSI * qAbs(m.B) * M_1_PI / sqrt(p);
    if (w2 < 0) return Double::nan();
    return sqrt(w2);
}

// TODO:NEXT-VER should be in Calculator as it can be reused by another functions, e.g. BeamParamsAtElems
double CausticFunction::calculateResonator_frontRadius(const Z::Matrix& m) const
{
    if (!Double(m.D).is(m.A))
        return 2 * m.B / (m.D - m.A);
    return (m.B < 0) ? -Double::infinity() : +Double::infinity();
}

// TODO:NEXT-VER should be in Calculator as it can be reused by another functions, e.g. BeamParamsAtElems
double CausticFunction::calculateResonator_halfAngle(const Z::Matrix& m) const
{
    double p = 4.0 - SQR(m.A + m.D);
    if (p <= 0) return Double::nan();
    return sqrt(_wavelenSI * M_1_PI * 2.0 * qAbs(m.C) / sqrt(p));
}

QString CausticFunction::calculatePoint(const double& arg)
{
    Q_UNUSED(arg)
    return QString();

/* TODO:NEXT-VER
    if (!_elem || !_calc || !_range.has(arg)) return QString();

    _elem->setSubRange(arg);
    _calc->multMatrix();

    Z::PointTS value;
    switch (_schema->tripType())
    {
    case TripType::SW: break; // TODO
    case TripType::RR: break; // TODO
    case TripType::SP: value = calculateSinglePass(); break;
    }

    return QString::fromUtf8("ω<sub>T</sub>: %1; ω<sub>S</sub>: %2")
            .arg(Z::format(value.T), Z::format(value.S)); */
}

void CausticFunction::convertFromSiToModeUnits(Z::PointTS& point) const
{
    switch (_mode)
    {
    case BeamRadius:
        point.T = _beamsizeUnit->fromSi(point.T);
        point.S = _beamsizeUnit->fromSi(point.S);
        break;

    case FontRadius:
        point.T = _curvatureUnit->fromSi(point.T);
        point.S = _curvatureUnit->fromSi(point.S);
        break;

    case HalfAngle:
        point.T = _angleUnit->fromSi(point.T);
        point.S = _angleUnit->fromSi(point.S);
        break;
    }
}

