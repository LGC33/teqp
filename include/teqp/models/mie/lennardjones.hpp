#pragma once

#include "teqp/models/multifluid.hpp"
#include "teqp/types.hpp"

namespace teqp {

    /**
     * The EOS of Monika Thol and colleagues. DOI:10.1063/1.4945000
     */
    inline auto build_LJ126_TholJPCRD2016() {
        std::string contents = R"(

        {
          "EOS": [
            {
              "BibTeX_CP0": "",
              "BibTeX_EOS": "Thol-THESIS-2015",
              "STATES": {
                "reducing": {
                  "T": 1.32,
                  "T_units": "LJ units",
                  "rhomolar": 0.31,
                  "rhomolar_units": "LJ units"
                }
              },
              "T_max": 1200,
              "T_max_units": "LJ units",
              "Ttriple": 0.661,
              "Ttriple_units": "LJ units",
              "alphar": [
                {
                  "d": [4, 1, 1, 2, 2, 3, 1, 1, 3, 2, 2, 5],
                  "l": [0, 0, 0, 0, 0, 0, 1, 2, 2, 1, 2, 1],
                  "n": [0.52080730e-2, 0.21862520e+1, -0.21610160e+1, 0.14527000e+1, -0.20417920e+1, 0.18695286e+0, -0.62086250e+0, -0.56883900e+0, -0.80055922e+0, 0.10901431e+0, -0.49745610e+0, -0.90988445e-1],
                  "t": [1.000, 0.320, 0.505, 0.672, 0.843, 0.898, 1.205, 1.786, 2.770, 1.786, 2.590, 1.294],
                  "type": "ResidualHelmholtzPower"
                },
                {
                  "beta": [0.625, 0.638, 3.91, 0.156, 0.157, 0.153, 1.16, 1.73, 383, 0.112, 0.119],
                  "d": [1, 1, 2, 3, 3, 2, 1, 2, 3, 1, 1],
                  "epsilon": [ 0.2053, 0.409, 0.6, 1.203, 1.829, 1.397, 1.39, 0.539, 0.934, 2.369, 2.43],
                  "eta": [2.067, 1.522, 8.82, 1.722, 0.679, 1.883, 3.925, 2.461, 28.2, 0.753, 0.82],
                  "gamma": [0.71, 0.86, 1.94, 1.48, 1.49, 1.945, 3.02, 1.11, 1.17, 1.33, 0.24],
                  "n": [-0.14667177e+1, 0.18914690e+1, -0.13837010e+0, -0.38696450e+0, 0.12657020e+0, 0.60578100e+0, 0.11791890e+1, -0.47732679e+0, -0.99218575e+1, -0.57479320e+0, 0.37729230e-2],
                  "t": [2.830, 2.548, 4.650, 1.385, 1.460, 1.351, 0.660, 1.496, 1.830, 1.616, 4.970],
                  "type": "ResidualHelmholtzGaussian"
                }
              ],
              "gas_constant": 1.0,
              "gas_constant_units": "LJ units",
              "molar_mass": 1.0,
              "molar_mass_units": "LJ units",
              "p_max": 100000,
              "p_max_units": "LJ units",
              "pseudo_pure": false
            }
          ],
          "INFO":{
            "NAME": "LennardJones",
            "REFPROP_NAME": "LJF",
            "CAS": "N/A"
            }
        }

        )";

        return teqp::build_multifluid_JSONstr({ contents }, "{}", "{}");
    };

    /**
    Jiri Kolafa and Ivo Nezbeda
    Fluid Phase Equilibria, 100 (1994) 1-34
    The Lennard-Jones fluid: An accurate analytic and theoretically-based equation of state
    doi: 10.1016/0378-3812(94)80001-4
     */
    class LJ126KolafaNezbeda1994{
    private:

        template<typename T>
        auto POW2(const T& x) const {
            return x*x;
        }

        template<typename T>
        auto POW3(const T& x) const{
            return POW2(x)*x;
        }

        const double MY_PI = EIGEN_PI;

        const std::vector<std::tuple<int, double>> c_dhBH  {
            {-2,  0.011117524},
            {-1, -0.076383859},
            { 0,  1.080142248},
            { 1,  0.000693129}
        };
        const double c_ln_dhBH = -0.063920968;

        const std::vector<std::tuple<int, double>> c_Delta_B2_hBH  {
            {-7, -0.58544978},
            {-6, 0.43102052},
            {-5, 0.87361369},
            {-4, -4.13749995},
            {-3, 2.90616279},
            {-2, -7.02181962},
            { 0, 0.02459877}
        };

        const std::vector<std::tuple<int, int, double>> c_Cij = {
            { 0, 2,    2.01546797},
            { 0, 3,  -28.17881636},
            { 0, 4,   28.28313847},
            { 0, 5,  -10.42402873},
            {-1, 2,  -19.58371655},
            {-1, 3,   75.62340289},
            {-1, 4, -120.70586598},
            {-1, 5,  93.92740328},
            {-1, 6, -27.37737354},
            {-2, 2, 29.34470520},
            {-2, 3, -112.3535693},
            {-2,  4, 170.64908980 },
            {-2, 5, -123.06669187},
            {-2, 6, 34.42288969 },
            {-4, 2, -13.37031968},
            {-4, 3, 65.38059570},
            {-4, 4, -115.09233113},
            {-4, 5, 88.91973082},
            {-4, 6, -25.62099890}
        };

        const double gamma = 1.92907278;

        // Form of Eq. 29
        template<typename TTYPE>
        auto get_dhBH(const TTYPE& Tstar) const {
            TTYPE summer = c_ln_dhBH*log(Tstar);
            for (auto [i, C_i] : c_dhBH){
                summer += C_i*pow(Tstar, i/2.0);
            }
            return forceeval(summer);
        }

        template<typename TTYPE>
        auto get_d_dhBH_d1T(const TTYPE& Tstar) const {
            TTYPE summer = c_ln_dhBH;
            for (auto [i, C_i] : c_dhBH){
                summer += (i/2.0)*C_i*pow(Tstar, i/2.0);
            }
            return forceeval(-Tstar*summer);
        }

        // Form of Eq. 29
        template<typename TTYPE>
        auto get_DeltaB2hBH(const TTYPE& Tstar) const {
            TTYPE summer = 0.0;
            for (auto [i, C_i] : c_Delta_B2_hBH){
                summer += C_i*pow(Tstar, i/2.0);
            }
            return forceeval(summer);
        }

        template<typename TTYPE>
        auto get_d_DeltaB2hBH_d1T(const TTYPE& Tstar) const {
            auto summer = 0.0;
            for (auto [i, C_i] : c_Delta_B2_hBH){
                summer += (i/2.0)*C_i*pow(Tstar, i/2.0);
            }
            return forceeval(-Tstar*summer);
        }

        //  Eq. 5 from K-N
        template<typename TTYPE, typename RHOTYPE>
        auto get_ahs(const TTYPE& Tstar, const RHOTYPE& rhostar) const {
            auto zeta = MY_PI/6.0*rhostar*pow(get_dhBH(Tstar), 3);
            return forceeval(Tstar*(5.0/3.0*log(1.0-zeta) + zeta*(34.0-33.0*zeta+4.0*POW2(zeta))/(6.0*POW2(1.0-zeta))));
        }

        // Eq. 4 from K-N
        template<typename TTYPE, typename RHOTYPE>
        auto get_zhs(const TTYPE& Tstar, const RHOTYPE& rhostar) const {
            std::common_type_t<TTYPE, RHOTYPE> zeta = MY_PI/6.0*rhostar*POW3(get_dhBH(Tstar));
            return forceeval((1.0+zeta+zeta**zeta-2.0/3.0*POW3(zeta)*(1+zeta))/POW3(1.0-zeta));
        }

        // Eq. 30 from K-N
        template<typename TTYPE, typename RHOTYPE>
        auto get_a(const TTYPE& Tstar, const RHOTYPE& rhostar) const{
            std::common_type_t<TTYPE, RHOTYPE> summer = 0.0;
            for (auto [i, j, Cij] : c_Cij){
                summer += Cij*pow(Tstar, i/2.0)*pow(rhostar, j);
            }
            return forceeval(get_ahs(Tstar, rhostar) + exp(-gamma*POW2(rhostar))*rhostar*Tstar*get_DeltaB2hBH(Tstar)+summer);
        }

    public:
        // We are in "simulation units", so R is 1.0, and T and rho that go into alphar are actually T^* and rho^*
        template<typename MoleFracType>
        double R(const MoleFracType &) const { return 1.0; }
        
        template<typename TTYPE, typename RHOTYPE, typename MoleFracType>
        auto alphar(const TTYPE& Tstar, const RHOTYPE& rhostar, const MoleFracType& /*molefrac*/) const {
            return forceeval(get_a(Tstar, rhostar)/Tstar);
        }
    };

    /**
     J. KARL JOHNSON, JOHN A. ZOLLWEG and KEITH E. GUBBINS
     The Lennard-Jones equation of state revisited
     MOLECULAR PHYSICS,1993, VOL. 78, No. 3, 591-618
     doi: 10.1080/00268979300100411
    */
    class LJ126Johnson1993 {
    
    private:
        template<typename T>
        auto POW2(const T& x) const -> T{
            return x*x;
        }

        template<typename T>
        auto POW3(const T& x) const -> T{
            return POW2(x)*x;
        }
        
        template<typename T>
        auto POW4(const T& x) const -> T{
            return POW2(x)*POW2(x);
        }
        
        const double gamma = 3.0;
        const std::vector<double> x {
            0.0, // placeholder for i=0 term since C++ uses 0-based indexing
            0.8623085097507421,
            2.976218765822098,
           -8.402230115796038,
            0.1054136629203555,
           -0.8564583828174598,
            1.582759470107601,
            0.7639421948305453,
            1.753173414312048,
            2.798291772190376e03,
           -4.8394220260857657e-2,
            0.9963265197721935,
           -3.698000291272493e01,
            2.084012299434647e01,
            8.305402124717285e01,
           -9.574799715203068e02,
           -1.477746229234994e02,
            
            6.398607852471505e01,
            1.603993673294834e01,
            6.805916615864377e01,
           -2.791293578795945e03,
           -6.245128304568454,
           -8.116836104958410e03,
            1.488735559561229e01,
           -1.059346754655084e04,
           -1.131607632802822e02,
           -8.867771540418822e03,
           -3.986982844450543e01,
           -4.689270299917261e03,
            2.593535277438717e02,
           -2.694523589434903e03,
           -7.218487631550215e02,
            1.721802063863269e02
        };
    
        // Table 5
        template<typename TTYPE>
        auto get_ai(const int i, const TTYPE& Tstar) const -> TTYPE{
            switch(i){
                case 1:
                    return x[1]*Tstar + x[2]*sqrt(Tstar) + x[3] + x[4]/Tstar + x[5]/POW2(Tstar);
                case 2:
                    return x[6]*Tstar + x[7] + x[8]/Tstar + x[9]/POW2(Tstar);
                case 3:
                    return x[10]*Tstar + x[11] + x[12]/Tstar;
                case 4:
                    return x[13];
                case 5:
                    return x[14]/Tstar + x[15]/POW2(Tstar);
                case 6:
                    return x[16]/Tstar;
                case 7:
                    return x[17]/Tstar + x[18]/POW2(Tstar);
                case 8:
                    return x[19]/POW2(Tstar);
                default:
                    throw teqp::InvalidArgument("i is not possible in get_ai");
            }
        }
        
        // Table 6
        template<typename TTYPE>
        auto get_bi(const int i, const TTYPE& Tstar) const -> TTYPE{
            switch(i){
                case 1:
                    return x[20]/POW2(Tstar) + x[21]/POW3(Tstar);
                case 2:
                    return x[22]/POW2(Tstar) + x[23]/POW4(Tstar);
                case 3:
                    return x[24]/POW2(Tstar) + x[25]/POW3(Tstar);
                case 4:
                    return x[26]/POW2(Tstar) + x[27]/POW4(Tstar);
                case 5:
                    return x[28]/POW2(Tstar) + x[29]/POW3(Tstar);
                case 6:
                    return x[30]/POW2(Tstar) + x[31]/POW3(Tstar) + x[32]/POW4(Tstar);
                default:
                    throw teqp::InvalidArgument("i is not possible in get_bi");
            }
        }
        
        // Table 7
        template<typename FTYPE, typename RHOTYPE>
        auto get_Gi(const int i, const FTYPE& F, const RHOTYPE& rhostar) const -> RHOTYPE{
            if (i == 1){
                return forceeval((1.0-F)/(2.0*gamma));
            }
            else{
                // Recursive definition of the other terms;
                return forceeval(-(F*powi(rhostar, 2*(i-1)) - 2.0*(i-1)*get_Gi(i-1, F, rhostar))/(2.0*gamma));
            }
        }
        
        template<typename TTYPE, typename RHOTYPE>
        auto get_alphar(const TTYPE& Tstar, const RHOTYPE& rhostar) const{
            std::common_type_t<TTYPE, RHOTYPE> summer = 0.0;
            auto F = exp(-gamma*POW2(rhostar));
            for (int i = 1; i <= 8; ++i){
                summer += get_ai(i, Tstar)*powi(rhostar, i)/static_cast<double>(i);
            }
            for (int i = 1; i <= 6; ++i){
                summer += get_bi(i, Tstar)*get_Gi(i, F, rhostar);
            }
            return forceeval(summer);
        }

    public:
    // We are in "simulation units", so R is 1.0, and T and rho that
    // go into alphar are actually T^* and rho^*
    template<typename MoleFracType>
    double R(const MoleFracType &) const { return 1.0; }
    
    template<typename TTYPE, typename RHOTYPE, typename MoleFracType>
    auto alphar(const TTYPE& Tstar, const RHOTYPE& rhostar, const MoleFracType& /*molefrac*/) const {
        return forceeval(get_alphar(Tstar, rhostar)/Tstar);
    }
        
    };

};
