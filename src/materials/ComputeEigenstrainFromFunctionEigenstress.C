//* This file is part of the RACCOON application
//* being developed at Dolbow lab at Duke University
//* http://dolbow.pratt.duke.edu

#include "ComputeEigenstrainFromFunctionEigenstress.h"

registerMooseObject("raccoonApp", ComputeEigenstrainFromFunctionEigenstress);
registerMooseObject("raccoonApp", ADComputeEigenstrainFromFunctionEigenstress);

template <bool is_ad>
InputParameters
ComputeEigenstrainFromFunctionEigenstressTempl<is_ad>::validParams()
{
  InputParameters params = Material::validParams();
  params.addClassDescription("Computes an eigenstrain from an eigen stress");
  params.addParam<std::string>("base_name",
                               "Optional parameter that allows the user to define "
                               "multiple mechanics material systems on the same "
                               "block, i.e. for multiple phases");
  params.addRequiredParam<std::string>("eigenstrain_name",
                                       "Material property name for the eigenstrain tensor computed "
                                       "by this model. IMPORTANT: The name of this property must "
                                       "also be provided to the strain calculator.");
  params.addRequiredParam<std::vector<FunctionName>>(
      "eigen_stress",
      "A list of functions describing the eigen stress.  There must be 9 of these, corresponding "
      "to the xx, yx, zx, xy, yy, zy, xz, yz, zz components respectively.");
  return params;
}

template <bool is_ad>
ComputeEigenstrainFromFunctionEigenstressTempl<
    is_ad>::ComputeEigenstrainFromFunctionEigenstressTempl(const InputParameters & parameters)
  : Material(parameters),
    _base_name(isParamValid("base_name") ? getParam<std::string>("base_name") + "_" : ""),
    _eigenstrain_name(_base_name + getParam<std::string>("eigenstrain_name")),
    _eigenstrain(declareGenericProperty<RankTwoTensor, is_ad>(_eigenstrain_name)),
    _elasticity_tensor(
        getGenericMaterialProperty<RankFourTensor, is_ad>(_base_name + "elasticity_tensor"))
{
  const std::vector<FunctionName> & fcn_names(getParam<std::vector<FunctionName>>("eigen_stress"));
  const std::size_t num = fcn_names.size();

  if (num != LIBMESH_DIM * LIBMESH_DIM)
    paramError("eigen_stress",
               name() + ": " + Moose::stringify(LIBMESH_DIM * LIBMESH_DIM) +
                   " initial stress functions must be provided.  You supplied " +
                   Moose::stringify(num) + ".");

  _eigen_stress_fcn.resize(num);
  for (unsigned i = 0; i < num; ++i)
    _eigen_stress_fcn[i] = &getFunctionByName(fcn_names[i]);
}

template <bool is_ad>
void
ComputeEigenstrainFromFunctionEigenstressTempl<is_ad>::initQpStatefulProperties()
{
  _eigenstrain[_qp].zero();
}

template <bool is_ad>
void
ComputeEigenstrainFromFunctionEigenstressTempl<is_ad>::computeQpProperties()
{
  GenericRankTwoTensor<is_ad> eigen_stress;
  for (unsigned i = 0; i < LIBMESH_DIM; ++i)
    for (unsigned j = 0; j < LIBMESH_DIM; ++j)
      eigen_stress(i, j) = _eigen_stress_fcn[i * LIBMESH_DIM + j]->value(_t, _q_point[_qp]);

  const GenericReal<is_ad> lambda = _elasticity_tensor[_qp](0, 0, 1, 1);
  const GenericReal<is_ad> G = _elasticity_tensor[_qp](0, 1, 0, 1);
  const GenericReal<is_ad> K = lambda + 2.0 * G / LIBMESH_DIM;
  const GenericRankTwoTensor<is_ad> I2(GenericRankTwoTensor<is_ad>::initIdentity);
  _eigenstrain[_qp] = -eigen_stress.trace() / 9 / K * I2 - eigen_stress.deviatoric() / 2 / G;
}

template class ComputeEigenstrainFromFunctionEigenstressTempl<false>;
template class ComputeEigenstrainFromFunctionEigenstressTempl<true>;
