#define _CRT_SECURE_NO_WARNINGS
#include <wx/wx.h>
#include <nlohmann/json.hpp>
#include <cpr/cpr.h>
#include <wx/stattext.h>
#include <wx/slider.h>
#include <wx/gauge.h>
#include <wx/combobox.h>
#include <string>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <fstream>
#include <map>
#include <windows.h>
#pragma comment(lib, "winmm.lib")

using json = nlohmann::json;

std::map<std::string, std::string> VOCES_ELEVENLABS = {
    {"Rachel", "21m00Tcm4TlvDq8ikWAM"},
    {"Domi", "AZnzlk1XvdvUeBnXmlld"},
    {"Bella", "EXAVITQu4vr4xnSDxMaL"},
    {"Antoni", "ErXwobaYiN019PkySvjV"}
};

std::string ELEVEN_API_KEY = "";
std::string OPENAI_KEY = "";
void CargarEnv(const std::string& ruta_archivo) {
    std::ifstream archivo(ruta_archivo);

    // Si no encuentra el archivo, lanzamos una alerta visual para que no pase desapercibido
    if (!archivo.is_open()) {
        wxMessageBox(wxString::FromUTF8("No se encontró el archivo .env. Verifica la ruta."), "Error de Archivo", wxICON_ERROR);
        return;
    }

    std::string linea;
    while (std::getline(archivo, linea)) {
        // Ignorar líneas vacías o comentarios
        if (linea.empty() || linea[0] == '#') continue;

        // Eliminar el salto de línea invisible de Windows (\r)
        if (!linea.empty() && linea.back() == '\r') {
            linea.pop_back();
        }

        auto delimitadorPos = linea.find('=');
        if (delimitadorPos != std::string::npos) {
            std::string clave = linea.substr(0, delimitadorPos);
            std::string valor = linea.substr(delimitadorPos + 1);

            // Eliminar comillas dobles si el usuario las puso en el .env
            if (!valor.empty() && valor.front() == '"') valor.erase(0, 1);
            if (!valor.empty() && valor.back() == '"') valor.pop_back();

            // Asignar los valores a las variables globales
            if (clave == "OPENAI_KEY") OPENAI_KEY = valor;
            else if (clave == "ELEVEN_API_KEY") ELEVEN_API_KEY = valor;
        }
    }
}
std::string TranscribirAudio(const std::string rutaArchivo){
    auto r = cpr::Post(
        cpr::Url("https://api.openai.com/v1/audio/transcriptions"),
        cpr::Header{
            {"Authorization", "Bearer " + OPENAI_KEY}
        },

        cpr::Multipart{{"file", cpr::File(rutaArchivo)},
        {"model", "whisper-1"} },
        cpr::VerifySsl{ false}

    );

    if (r.status_code == 200){
        auto datos = json::parse(r.text);
        return datos["text"].get<std::string>();
    }else{
        return "Error al escuchar: " + std::to_string(r.status_code);
    }

}
std::string Preguntar_IA(std::string preguntar) {
    // 1. Estructura JSON súper segura
    json modelo_openai;
    modelo_openai["model"] = "gpt-4o-mini";
    modelo_openai["messages"] = json::array({
        {{"role", "user"}, {"content", preguntar}}
    });

    // 2. Hacemos la petición
    auto r = cpr::Post(
        cpr::Url{ "https://api.openai.com/v1/chat/completions" },
        cpr::Header{ {"Authorization", "Bearer " + OPENAI_KEY}, {"Content-Type", "application/json"} },
        cpr::Body{ modelo_openai.dump() },
        cpr::VerifySsl{ false } // Lo ponemos en false por si Windows bloquea el certificado
    );

    // 3. Manejo de respuesta o error detallado
    if (r.status_code == 200) {
        auto respuesta = json::parse(r.text);
        return respuesta["choices"][0]["message"]["content"].get<std::string>();
    } else {
        // ¡ESTO ES CLAVE! Si hay error, mostrará exactamente de qué se queja OpenAI
        return "Error " + std::to_string(r.status_code) + " - Detalle: " + r.text;
    }
}




    std::string TextAudi (std::string texto, std::string nombresVoces){
        std::string voz_id = VOCES_ELEVENLABS[nombresVoces];
        std::string output = "Respuesta" + std::to_string(time(0)) + ".mp3";
        json modelo_eleven = {{"text", texto}, {"model_id", "eleven_multilingual_v2"}};
        auto r = cpr::Post(
          cpr::Url{"https://api.elevenlabs.io/v1/text-to-speech/" + voz_id},
          cpr::Header{ {"Content-Type", "application/json"}, {"xi-api-key", ELEVEN_API_KEY} },
          cpr::Body{ modelo_eleven.dump() }, cpr::VerifySsl{ false }
        );
        if (r.status_code == 200 ){
            std::ofstream f(output, std::ios::binary);
            f.write(r.text.c_str(), r.text.size());
            return output;
        }
        return "Error al generar audio";
    }


    class VentanaPrincipal : public wxFrame{
    public:
        VentanaPrincipal() : wxFrame(NULL, wxID_ANY, "Asistente IA", wxDefaultPosition, wxSize(800, 500)){
            wxPanel* panel = new wxPanel(this);
            panel->SetBackgroundColour(wxColour("#eef2f3"));
            wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);
            wxBoxSizer* leftSizer = new wxBoxSizer(wxVERTICAL);

            wxStaticText* titulo = new wxStaticText(panel, wxID_ANY, "Asistente IA");
            titulo->SetFont(wxFont(16, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
            leftSizer->Add(titulo, 0, wxALL | wxALIGN_CENTER, 15);

            leftSizer->Add(new wxStaticText(panel, wxID_ANY, wxString::FromUTF8("Duración de la grabación")), 0, wxTOP | wxALIGN_CENTER, 10);
            sliderDuracion = new wxSlider(panel, wxID_ANY, 5, 1, 15, wxDefaultPosition, wxSize(250, -1), wxSL_HORIZONTAL | wxSL_LABELS);
            leftSizer->Add(sliderDuracion, 0, wxALL | wxALIGN_CENTER, 5);

            leftSizer->Add(new wxStaticText(panel, wxID_ANY, "Selecciona voz:"), 0, wxTOP | wxALIGN_CENTER, 10);
            wxArrayString voces;
            for (auto const& [name, id] : VOCES_ELEVENLABS) voces.Add(name);
            selectorVoz = new wxComboBox(panel, wxID_ANY, voces[0], wxDefaultPosition, wxSize(250, -1), voces, wxCB_READONLY);
            leftSizer->Add(selectorVoz, 0, wxALL | wxALIGN_CENTER, 5);

            btnProcesar = new wxButton(panel, wxID_ANY, "Procesar y Hablar", wxDefaultPosition, wxSize(200, 40));
            btnProcesar->SetBackgroundColour(wxColour("#4A90E2"));
            btnProcesar->SetForegroundColour(*wxWHITE);
            btnProcesar->Bind(wxEVT_BUTTON, &VentanaPrincipal::OnProcesar, this);
            leftSizer->Add(btnProcesar, 0, wxTOP | wxALIGN_CENTER, 20);
            progreso = new wxGauge(panel, wxID_ANY, 100, wxDefaultPosition, wxSize(250, 20));
            leftSizer->Add(progreso, 0, wxALL | wxALIGN_CENTER, 15);

            wxBoxSizer* rightSizer = new wxBoxSizer(wxVERTICAL);
            rightSizer->Add(new wxStaticText(panel, wxID_ANY, "Historial de conversacion", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER), 0, wxEXPAND | wxTOP, 15);
            historialBox = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
            rightSizer->Add(historialBox, 1, wxEXPAND | wxALL, 15);

            mainSizer->Add(leftSizer, 0, wxEXPAND | wxALL, 10);
            mainSizer->Add(rightSizer, 1, wxEXPAND | wxALL, 0);

            panel->SetSizer(mainSizer);
            this->Centre();

            leftSizer->Add(new wxStaticText(panel, wxID_ANY, "O escribe tu mensaje:"), 0, wxTOP | wxALIGN_CENTER, 15);

            inputTexto = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxSize(250, -1), wxTE_PROCESS_ENTER);
            leftSizer->Add(inputTexto, 0, wxALL | wxALIGN_CENTER, 5);

            btnEnviarTexto = new wxButton(panel, wxID_ANY, "Enviar Texto", wxDefaultPosition, wxSize(200, 30));
            btnEnviarTexto->SetBackgroundColour(wxColour("#2ECC71")); // Color verde para diferenciar
            btnEnviarTexto->SetForegroundColour(*wxWHITE);
            btnEnviarTexto->Bind(wxEVT_BUTTON, &VentanaPrincipal::OnEnviarTexto, this);
            leftSizer->Add(btnEnviarTexto, 0, wxTOP | wxALIGN_CENTER, 5);


        }

    private:
        wxSlider* sliderDuracion;
        wxGauge* progreso;
        wxComboBox* selectorVoz;
        wxTextCtrl* historialBox;
        wxButton* btnProcesar;
        wxTextCtrl* inputTexto;
        wxButton* btnEnviarTexto;

        std::string TextoAAudio(std::string texto, std::string voiceName) {
            std::string voiceId = VOCES_ELEVENLABS[voiceName];
            std::string output = "respuesta_" + std::to_string(time(0)) + ".mp3";
            json modelo_eleven = { {"text", texto}, {"model_id", "eleven_multilingual_v2"} };

            auto r = cpr::Post(cpr::Url{ "https://api.elevenlabs.io/v1/text-to-speech/" + voiceId },
                cpr::Header{ {"Content-Type", "application/json"}, {"xi-api-key", ELEVEN_API_KEY} },
                cpr::Body{ modelo_eleven.dump() }, cpr::VerifySsl{ false });

            if (r.status_code == 200) {
                std::ofstream f(output, std::ios::binary);
                f.write(r.text.c_str(), r.text.size());
                return output;
            }
            return "";
        }
    void GrabarAudio(int segundos, const std::string& nombreArchivo) {

            mciSendStringA("open new type waveaudio alias mic", NULL, 0, NULL);

            mciSendStringA("record mic", NULL, 0, NULL);


            for (int i = 0; i < segundos * 10; i++) {
                wxMilliSleep(100);
                wxYield();
            }


            std::string comandoGuardar = "save mic " + nombreArchivo;
            mciSendStringA(comandoGuardar.c_str(), NULL, 0, NULL);
            mciSendStringA("close mic", NULL, 0, NULL);
        }

    void OnProcesar(wxCommandEvent& event) {
            progreso->SetValue(0);


            int segundos = sliderDuracion->GetValue();
            historialBox->AppendText("Escuchando por " + std::to_string(segundos) + " segundos...\n");


            GrabarAudio(segundos, "grabacion.wav");

            historialBox->AppendText("Procesando tu voz...\n");


            std::string pregunta = TranscribirAudio("grabacion.wav");
            historialBox->AppendText("Usuario: " + wxString::FromUTF8(pregunta) + "\n");

            std::string respuesta = Preguntar_IA(pregunta);
            historialBox->AppendText("Asistente: " + wxString::FromUTF8(respuesta) + "\n");


            std::string voz = selectorVoz->GetStringSelection().ToStdString();
            std::string audio = TextoAAudio(respuesta, voz);


            if (!audio.empty()) {
                ShellExecuteA(NULL, "open", audio.c_str(), NULL, NULL, SW_HIDE);
            }

            progreso->SetValue(100);
            historialBox->AppendText("--------------------------\n\n");
        }

        void ProcesarRespuestaGeneral(std::string prompt) {
            if (prompt.empty()) return;

            std::string respuesta = Preguntar_IA(prompt);
            historialBox->AppendText("Asistente: " + wxString::FromUTF8(respuesta) + "\n");


            std::string voz = selectorVoz->GetStringSelection().ToStdString();
            std::string audio = TextoAAudio(respuesta, voz);

            if (!audio.empty()) {
                ShellExecuteA(NULL, "open", audio.c_str(), NULL, NULL, SW_HIDE);
            }

            progreso->SetValue(100);
            historialBox->AppendText("--------------------------\n\n");
        }
    void OnEnviarTexto(wxCommandEvent& event) {

            std::string consulta = std::string(inputTexto->GetValue().utf8_str());
            if (consulta.empty()) return;

            inputTexto->Clear();
            ProcesarRespuestaGeneral(consulta);
        }


    };

class MyApp : public wxApp {
public:
    virtual bool OnInit() {



        (new VentanaPrincipal())->Show();
        return true;
    }
};
    wxIMPLEMENT_APP(MyApp);
