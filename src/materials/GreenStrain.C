#include "GreenStrain.h"

registerADMooseObject("raccoonApp", GreenStrain);

defineADValidParams(GreenStrain,
                    ADComputeStrainBase,
                    params.addClassDescription("Compute a Green strain."););

template <ComputeStage compute_stage>
GreenStrain<compute_stage>::GreenStrain(const InputParameters & parameters)
  : ADComputeStrainBase<compute_stage>(parameters),
    _F(adDeclareADProperty<RankTwoTensor>(_base_name + "deformation_gradient"))
{
}

template <ComputeStage compute_stage>
void
GreenStrain<compute_stage>::computeQpProperties()
{
  // deformation gradient
  // F = I + A
  //         A = U_{i,I}, displacement gradient in reference configuration
  ADRankTwoTensor A((*_grad_disp[0])[_qp], (*_grad_disp[1])[_qp], (*_grad_disp[2])[_qp]);
  _F[_qp] = A;
  _F[_qp].addIa(1.0);

  // Green strain defined in the reference configuration
  // E = 0.5(F^T F - I)
  ADRankTwoTensor E = _F[_qp].transpose() * _F[_qp];
  E.addIa(-1.0);
  E *= 0.5;

  // total strain defined in the current configuration
  // e = F E F^T / det(F)
  _total_strain[_qp] = E;
  if (_global_strain)
    _total_strain[_qp] += (*_global_strain)[_qp];

  // mechanical strain in the current configuration
  _mechanical_strain[_qp] = _total_strain[_qp];
  for (auto es : _eigenstrains)
    _mechanical_strain[_qp] -= (*es)[_qp];
}