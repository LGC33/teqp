/***
 * Although this is a C-language interface, the code itself is of course written in C++
 * to allow for features only available in C++ and avoiding C memory management
 */

#include <unordered_map>
#include <variant>
#include <atomic>

#include "teqp/cpp/teqpcpp.hpp"
#include "teqp/exceptions.hpp"

// Define empty macros so that no exporting happens
#if defined(TEQPC_CATCH)
#define EXPORT_CODE
#define CONVENTION

#else

#if defined(EXTERN_C_DLLEXPORT)
#define EXPORT_CODE extern "C" __declspec(dllexport) 
#endif
#if defined(EXTERN_C)
#define EXPORT_CODE extern "C" 
#endif
#ifndef CONVENTION
#  define CONVENTION
#endif
#endif

using namespace teqp;

// An atomic is used here for thread safety
// The max possible index is 18,446,744,073,709,551,615
std::atomic<long long int> next_index{ 0 };

std::unordered_map<unsigned long long int, std::shared_ptr<teqp::cppinterface::AbstractModel>> library;

void exception_handler(int& errcode, char* message_buffer, const int buffer_length)
{
    try{
        throw; // Rethrow the error so that we can handle it here
    }
    catch (teqpcException& e) {
        errcode = e.code;
        strcpy(message_buffer, e.msg.c_str());
    }
    catch (std::exception e) {
        errcode = 9999;
        strcpy(message_buffer, e.what());
    }
}

EXPORT_CODE int CONVENTION build_model(const char* j, long long int* uuid, char* errmsg, int errmsg_length){
    int errcode = 0;
    try{
        nlohmann::json json = nlohmann::json::parse(j);
        long long int uid = next_index++;
        try {
            library.emplace(std::make_pair(uid, cppinterface::make_model(json)));
        }
        catch (std::exception &e) {
            throw teqpcException(30, "Unable to load with error:" + std::string(e.what()));
        }
        *uuid = uid;
    }
    catch (...) {
        exception_handler(errcode, errmsg, errmsg_length);
    }
    return errcode;
}

EXPORT_CODE int CONVENTION free_model(const long long int uuid, char* errmsg, int errmsg_length) {
    int errcode = 0;
    try {
        library.erase(uuid);
    }
    catch (...) {
        exception_handler(errcode, errmsg, errmsg_length);
    }
    return errcode;
}

EXPORT_CODE int CONVENTION get_Arxy(const long long int uuid, const int NT, const int ND, const double T, const double rho, const double* molefrac, const int Ncomp, double *val, char* errmsg, int errmsg_length) {
    int errcode = 0;
    try {
        // Make an Eigen view of the double buffer
        Eigen::Map<const Eigen::ArrayXd> molefrac_(molefrac, Ncomp);
        // Call the function
        *val = library.at(uuid)->get_Arxy(NT, ND, T, rho, molefrac_);
    }
    catch (...) {
        exception_handler(errcode, errmsg, errmsg_length);
    }
    return errcode;
}

#if defined(TEQPC_CATCH)

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark_all.hpp>

#include "teqp/json_tools.hpp"

TEST_CASE("Use of C interface","[teqpc]") {

    constexpr int errmsg_length = 300;
    long long int uuid, uuidPR, uuidMF;
    char errmsg[errmsg_length] = "";
    double val = -1;
    std::valarray<double> molefrac = { 1.0 };

    std::string j = R"(
            {
                "kind": "PR", 
                "model": {
                    "Tcrit / K": [190], 
                    "pcrit / Pa": [3.5e6], 
                    "acentric": [0.11]
                }
            }
        )";
    build_model(j.c_str(), &uuidPR, errmsg, errmsg_length);
    {
        nlohmann::json jmodel = nlohmann::json::object();
        jmodel["departure"] = "";
        jmodel["BIP"] = "";
        jmodel["components"] = nlohmann::json::array();
        jmodel["components"].push_back("../mycp/dev/fluids/Argon.json");

        nlohmann::json j = {
            {"kind", "multifluid"},
            {"model", jmodel}
        };
        std::string js = j.dump(2);
        int e1 = build_model(js.c_str(), &uuidMF, errmsg, errmsg_length);
    }
    {
        nlohmann::json jmodel = nlohmann::json::object();
        jmodel["departure"] = "";
        jmodel["BIP"] = "";
        jmodel["components"] = nlohmann::json::array();
        jmodel["components"].push_back(load_a_JSON_file("../mycp/dev/fluids/Argon.json"));
        
        nlohmann::json j = {
            {"kind", "multifluid"},
            {"model", jmodel}
        };
        std::string js = j.dump(2);
        int e1 = build_model(js.c_str(), &uuid, errmsg, errmsg_length);
        CAPTURE(errmsg);
        REQUIRE(e1 == 0);
    }
    {
        nlohmann::json jmodel = nlohmann::json::object();
        jmodel["departure"] = "";
        jmodel["BIP"] = "";
        jmodel["components"] = nlohmann::json::array();
        jmodel["components"].push_back("Ethane");
        jmodel["components"].push_back("Nitrogen");
        jmodel["root"] = "../mycp";
        
        nlohmann::json j = {
            {"kind", "multifluid"},
            {"model", jmodel}
        };
        std::string js = j.dump(2);
        int e1 = build_model(js.c_str(), &uuid, errmsg, errmsg_length);
        REQUIRE(e1 == 0);
    }
    
    BENCHMARK("vdW1 parse string") {
        std::string j = R"({"kind":"vdW1", "model":{"a":1.0, "b":2.0}})";
        return nlohmann::json::parse(j);
    };
    BENCHMARK("vdW1 construct only") {
        std::string j = R"({"kind":"vdW1", "model":{"a":1.0, "b":2.0}})";
        int e1 = build_model(j.c_str(), &uuid, errmsg, errmsg_length);
        return e1;
    };
    BENCHMARK("vdW1") {
        std::string j = R"({"kind":"vdW1", "model":{"a":1.0, "b":2.0}})";
        int e1 = build_model(j.c_str(), &uuid, errmsg, errmsg_length);
        int e2 = get_Arxy(uuid, 0, 0, 300.0, 3.0e-6, &(molefrac[0]), static_cast<int>(molefrac.size()), &val, errmsg, errmsg_length);
        int e3 = free_model(uuid, errmsg, errmsg_length);
        REQUIRE(e1 == 0);
        REQUIRE(e2 == 0);
        REQUIRE(e3 == 0);
        return val;
    };
    BENCHMARK("PR") {
        std::string j = R"(
            {
                "kind": "PR", 
                "model": {
                    "Tcrit / K": [190], 
                    "pcrit / Pa": [3.5e6], 
                    "acentric": [0.11]
                }
            }
        )";
        int e1 = build_model(j.c_str(), &uuid, errmsg, errmsg_length);
        int e2 = get_Arxy(uuid, 0, 0, 300.0, 3.0e-6, &(molefrac[0]), static_cast<int>(molefrac.size()), &val, errmsg, errmsg_length);
        int e3 = free_model(uuid, errmsg, errmsg_length);
        REQUIRE(e1 == 0);
        REQUIRE(e2 == 0);
        REQUIRE(e3 == 0);
        return val;
    };
    BENCHMARK("PR call") {
        int e = get_Arxy(uuidPR, 0, 0, 300.0, 3.0e-6, &(molefrac[0]), static_cast<int>(molefrac.size()), &val, errmsg, errmsg_length);
        REQUIRE(e == 0);
        return val;
    };
    BENCHMARK("SRK") {
        std::string j = R"(
            {
                "kind": "SRK", 
                "model": {
                    "Tcrit / K": [190], 
                    "pcrit / Pa": [3.5e6], 
                    "acentric": [0.11]
                }
            }
        )";
        int e1 = build_model(j.c_str(), &uuid, errmsg, errmsg_length);
        int e2 = get_Arxy(uuid, 0, 1, 300.0, 3.0e-6, &(molefrac[0]), static_cast<int>(molefrac.size()), &val, errmsg, errmsg_length);
        int e3 = free_model(uuid, errmsg, errmsg_length);
        REQUIRE(e1 == 0);
        REQUIRE(e2 == 0);
        REQUIRE(e3 == 0);
        return val;
    };

    BENCHMARK("CPA") {
        nlohmann::json water = {
            {"a0i / Pa m^6/mol^2",0.12277 }, {"bi / m^3/mol", 0.000014515}, {"c1", 0.67359}, {"Tc / K", 647.096},
            {"epsABi / J/mol", 16655.0}, {"betaABi", 0.0692}, {"class", "4C"}
        };
        nlohmann::json jCPA = {
            {"cubic","SRK"},
            {"pures", {water}},
            {"R_gas / J/mol/K", 8.3144598}
        };
        nlohmann::json j = {
            {"kind", "CPA"},
            {"model", jCPA}
        };
        std::string jstring = j.dump();
        int e1 = build_model(jstring.c_str(), &uuid, errmsg, errmsg_length);
        int e2 = get_Arxy(uuid, 0, 1, 300.0, 3.0e-6, &(molefrac[0]), static_cast<int>(molefrac.size()), &val, errmsg, errmsg_length);
        int e3 = free_model(uuid, errmsg, errmsg_length);
        REQUIRE(e1 == 0);
        REQUIRE(e2 == 0);
        REQUIRE(e3 == 0);
        return val;
    };

    BENCHMARK("PCSAFT") {
        nlohmann::json jcoeffs = nlohmann::json::array();
        std::valarray<double> molefrac = { 0.4, 0.6 };
        jcoeffs.push_back({ {"name", "Methane"}, { "m", 1.0 }, { "sigma_Angstrom", 3.7039},{"epsilon_over_k", 150.03}, {"BibTeXKey", "Gross-IECR-2001"} });
        jcoeffs.push_back({ {"name", "Ethane"}, { "m", 1.6069 }, { "sigma_Angstrom", 3.5206},{"epsilon_over_k", 191.42}, {"BibTeXKey", "Gross-IECR-2001"} });
        nlohmann::json model = {
            {"coeffs", jcoeffs}
        };
        nlohmann::json j = {
            {"kind", "PCSAFT"},
            {"model", model}
        };
        std::string js = j.dump(1);
        int e1 = build_model(js.c_str(), &uuid, errmsg, errmsg_length);
        CAPTURE(errmsg);
        CAPTURE(js);
        REQUIRE(e1 == 0);
        int e2 = get_Arxy(uuid, 0, 1, 300.0, 3.0e-6, &(molefrac[0]), static_cast<int>(molefrac.size()), &val, errmsg, errmsg_length);
        int e3 = free_model(uuid, errmsg, errmsg_length);
        REQUIRE(e2 == 0);
        REQUIRE(e3 == 0);
        return val;
    };

    BENCHMARK("multifluid pure with fluid path") {
        nlohmann::json jmodel = nlohmann::json::object();
        jmodel["departure"] = nlohmann::json::array();
        jmodel["BIP"] = nlohmann::json::array();
        jmodel["components"] = nlohmann::json::array();
        jmodel["components"].push_back("../mycp/dev/fluids/Argon.json");
        
        nlohmann::json j = {
            {"kind", "multifluid"},
            {"model", jmodel}
        };
        std::string js = j.dump(2);
        int e1 = build_model(js.c_str(), &uuid, errmsg, errmsg_length);
        int e2 = get_Arxy(uuid, 0, 1, 300.0, 3.0e-6, &(molefrac[0]), static_cast<int>(molefrac.size()), &val, errmsg, errmsg_length);
        int e3 = free_model(uuid, errmsg, errmsg_length);
        REQUIRE(e1 == 0);
        REQUIRE(e2 == 0);
        REQUIRE(e3 == 0);
        return val;
    };

    BENCHMARK("multifluid pure with fluid contents") {
        nlohmann::json jmodel = nlohmann::json::object();
        jmodel["components"] = nlohmann::json::array();
        jmodel["components"].push_back(load_a_JSON_file("../mycp/dev/fluids/Argon.json")); 
        jmodel["departure"] = nlohmann::json::array();
        jmodel["BIP"] = nlohmann::json::array();
        jmodel["flags"] = nlohmann::json::object();

        nlohmann::json j = {
            {"kind", "multifluid"},
            {"model", jmodel}
        };
        std::string js = j.dump(2);
        int e1 = build_model(js.c_str(), &uuid, errmsg, errmsg_length);
        int e2 = get_Arxy(uuid, 0, 1, 300.0, 3.0e-6, &(molefrac[0]), static_cast<int>(molefrac.size()), &val, errmsg, errmsg_length);
        int e3 = free_model(uuid, errmsg, errmsg_length);
        REQUIRE(e1 == 0);
        REQUIRE(e2 == 0);
        REQUIRE(e3 == 0);
        return val;
    };

    BENCHMARK("multifluid call") {
        int e2 = get_Arxy(uuidMF, 0, 1, 300.0, 3.0e-6, &(molefrac[0]), static_cast<int>(molefrac.size()), &val, errmsg, errmsg_length);
        REQUIRE(e2 == 0);
        return val;
    };
    
}
#else 
int main() {
}
#endif
