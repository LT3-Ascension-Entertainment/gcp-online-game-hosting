// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "DroidshooterIntroUserWidget.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Droidshooter.h"
#include "Json.h"

void UDroidshooterIntroUserWidget::NativePreConstruct() 
{
    Super::NativePreConstruct();
    // B_Auth->OnClicked.AddDynamic(this, &UDroidshooterIntroUserWidget::AuthenticateCall);
}

void UDroidshooterIntroUserWidget::FindPreferredGameServerLocation(const FString& frontendApi, const FString& accessToken)
{
    // Query game server list
    // Ping each server
    // Create timer to check if all values have been updated
    // Save best region
    // Query FetchGameServer 

    UE_LOG(LogDroidshooter, Log, TEXT("Fetch regions to ping with token: %s"), *accessToken);

    FString uriPlay = frontendApi + TEXT("/ping");

    FHttpModule& httpModule = FHttpModule::Get();
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> pRequest = httpModule.CreateRequest();

    pRequest->SetHeader("Authorization", "Bearer " + accessToken);
    pRequest->SetVerb(TEXT("GET"));
    pRequest->SetURL(uriPlay);

    // Set the callback, which will execute when the HTTP call is complete
    pRequest->OnProcessRequestComplete().BindLambda(
        [&](
            FHttpRequestPtr pRequest,
            FHttpResponsePtr pResponse,
            bool connectedSuccessfully) mutable {

                if (connectedSuccessfully) {
                    ProcessServersToPingResponse(pResponse->GetContentAsString());
                }
                else {
                    switch (pRequest->GetStatus()) {
                    case EHttpRequestStatus::Failed_ConnectionError:
                        UE_LOG(LogDroidshooter, Log, TEXT("Connection failed."));
                    default:
                        UE_LOG(LogDroidshooter, Log, TEXT("Request failed."));
                    }
                }
        });

    // Finally, submit the request for processing
    pRequest->ProcessRequest();
}

void UDroidshooterIntroUserWidget::AuthenticateCall(const FString& frontendApi, const FString& accessToken)
{
    UE_LOG(LogDroidshooter, Log, TEXT("Auth call with token: %s"), *accessToken);

	FString uriProfile = frontendApi + TEXT("/profile");

	FHttpModule& httpModule = FHttpModule::Get();
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> pRequest = httpModule.CreateRequest();

    pRequest->SetVerb(TEXT("GET"));
    pRequest->SetHeader("Authorization", "Bearer " + accessToken);
    pRequest->SetURL(uriProfile);

    pRequest->OnProcessRequestComplete().BindLambda(
        [&](
            FHttpRequestPtr pRequest,
            FHttpResponsePtr pResponse,
            bool connectedSuccessfully) mutable {

                if (connectedSuccessfully) {
                    ProcessProfileResponse(pResponse->GetContentAsString());
                }
                else {
                    switch (pRequest->GetStatus()) {
                    case EHttpRequestStatus::Failed_ConnectionError:
                        UE_LOG(LogDroidshooter, Log, TEXT("Connection failed."));
                    default:
                        UE_LOG(LogDroidshooter, Log, TEXT("Request failed."));
                    }
                }
        });

    // Finally, submit the request for processing
    pRequest->ProcessRequest();
}

void UDroidshooterIntroUserWidget::FetchGameServer(const FString& frontendApi, const FString& accessToken, std::map<float, FString> servers)
{
    UE_LOG(LogDroidshooter, Log, TEXT("Fetch server/ip call with token: %s"), *accessToken);
    
    TSharedRef<FJsonObject> JsonRootObject = MakeShareable(new FJsonObject);
    TSharedRef<FJsonObject> JsonBodyObject = MakeShareable(new FJsonObject);
    TArray<TSharedPtr<FJsonValue>>  JsonServerArray;

    for (auto it = servers.begin(); it != servers.end(); ++it)
    {
        UE_LOG(LogDroidshooter, Log, TEXT("Ping responses: %.2f %s "), it->first, *it->second);
        JsonBodyObject->Values.Add(it->second, MakeShareable(new FJsonValueNumber(static_cast<int32>(it->first))));
    }

    JsonRootObject->SetObjectField("pingByRegion", JsonBodyObject);    

    FString OutputString;
    TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonRootObject, Writer);

    FString uriPlay = frontendApi + TEXT("/play");

    FHttpModule& httpModule = FHttpModule::Get();
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> pRequest = httpModule.CreateRequest();

    pRequest->SetHeader("Authorization", "Bearer " + accessToken);
    pRequest->SetVerb(TEXT("POST"));
    pRequest->SetURL(uriPlay);
    pRequest->SetHeader(TEXT("User-Agent"), "X-UnrealEngine-Agent");
    pRequest->SetHeader("Content-Type", TEXT("application/json"));
    pRequest->SetHeader(TEXT("Accepts"), TEXT("application/json"));

    pRequest->SetContentAsString(OutputString);

    // Set the callback, which will execute when the HTTP call is complete
    pRequest->OnProcessRequestComplete().BindLambda(
        [&](
            FHttpRequestPtr pRequest,
            FHttpResponsePtr pResponse,
            bool connectedSuccessfully) mutable {

                if (connectedSuccessfully) {
                    /*std::function<void(const TSharedPtr<FJsonObject>&)> f = [=](const TSharedPtr<FJsonObject>& JsonResponseObject) {
                        // do stuff here or call a method
                    };*/
                    ProcessGameserverResponse(pResponse->GetContentAsString());
                }
                else {
                    switch (pRequest->GetStatus()) {
                    case EHttpRequestStatus::Failed_ConnectionError:
                        UE_LOG(LogDroidshooter, Log, TEXT("Connection failed."));
                    default:
                        UE_LOG(LogDroidshooter, Log, TEXT("Request failed."));
                    }
                }
        });

    // Finally, submit the request for processing
    pRequest->ProcessRequest();
}

void UDroidshooterIntroUserWidget::ProcessProfileResponse(const FString& ResponseContent)
{
    TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ResponseContent);
    TSharedPtr<FJsonObject> JsonResponseObject;

    if (FJsonSerializer::Deserialize(JsonReader, JsonResponseObject))
    {
        if (JsonResponseObject)
        {
            FString Name = JsonResponseObject->GetStringField(TEXT("player_name"));

            if (Name.Len() != 0) {
                //UE_LOG(LogDroidshooter, Log, TEXT("Logged in player is: %s"), "yes");
                UE_LOG(LogDroidshooter, Log, TEXT("Logged in player is: %s"), *Name);

                NameTextBlock->SetText(FText::FromString(Name));
            }
            else {
                UE_LOG(LogDroidshooter, Log, TEXT("Unable to fetch player data. Timed out token?"));
            }

        }
    }
}

void UDroidshooterIntroUserWidget::ProcessGameserverResponse(const FString& ResponseContent) 
{
    TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ResponseContent);
    TSharedPtr<FJsonObject> JsonResponseObject;

    if (FJsonSerializer::Deserialize(JsonReader, JsonResponseObject))
    {
        if (JsonResponseObject)
        {
            FString IP = JsonResponseObject->GetStringField(TEXT("IP"));
            FString Port = JsonResponseObject->GetStringField(TEXT("Port"));

            if (IP.Len() != 0 && Port.Len() != 0) {
                // Set IP - Port variables!
                ServerIPValue = IP;
                ServerPortValue = Port;

                ServerIPBox->SetText(FText::FromString(IP));
                ServerPortBox->SetText(FText::FromString(Port));

                UE_LOG(LogDroidshooter, Log, TEXT("Found game server at: %s %s"), *IP, *Port);

                UFunction* Function = this->FindFunction("ConnectToOnlineGame");

                if (Function == nullptr)
                {
                    return;
                }

                this->ProcessEvent(Function, {});
            }
            else {
                UE_LOG(LogDroidshooter, Log, TEXT("Unable to server player data. Timed out token?"));
            }
        }
    }
}

void UDroidshooterIntroUserWidget::ProcessServersToPingResponse(const FString& ResponseContent)
{

    TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ResponseContent);
    TArray<TSharedPtr<FJsonValue>> JsonResponseArray;
    TSharedPtr<FJsonObject> JsonTopObject;

    if (FJsonSerializer::Deserialize(JsonReader, JsonTopObject))
    {
        ServerPinger.SetServersToValidate(JsonTopObject->Values.Num());

        for (auto JsonResponseObject = JsonTopObject->Values.CreateConstIterator(); JsonResponseObject; ++JsonResponseObject) {
            const FString KName = (*JsonResponseObject).Key;

            // Get the value as a FJsonValue object
            TSharedPtr< FJsonValue > Value = (*JsonResponseObject).Value;

            TSharedPtr<FJsonObject> JsonObjectIn = Value->AsObject();

            if (JsonObjectIn)
            {
                FString Name = JsonObjectIn->GetStringField(TEXT("Name"));
                FString Region = JsonObjectIn->GetStringField(TEXT("Region"));
                FString Address = JsonObjectIn->GetStringField(TEXT("Address"));
                FString Protocol = JsonObjectIn->GetStringField(TEXT("Protocol"));
                FString Port = JsonObjectIn->GetStringField(TEXT("Port"));

                UE_LOG(LogDroidshooter, Log, TEXT("Gonna ping: %s %s %s %s %s (%s)"), *Name, *Region, *Address, *Protocol, *Port, *KName);
                ServerPinger.CheckIfServerIsOnline(Address, Port, KName);
            }
        }
    }

    GetWorld()->GetTimerManager().SetTimer(MemberTimerHandle, this, &UDroidshooterIntroUserWidget::AllServersValidated, 1.0f, true, 3.0f);
}

/*
* Generic handler for json that calls func() passed to it
*/
void UDroidshooterIntroUserWidget::ProcessGenericJsonResponse(const FString& ResponseContent, std::function<void(const TSharedPtr<FJsonObject>&)>& func)
{
    TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ResponseContent);
    TSharedPtr<FJsonObject> JsonObject;

    if (FJsonSerializer::Deserialize(JsonReader, JsonObject))
    {
        func(JsonObject);
    }
}

void UDroidshooterIntroUserWidget::AllServersValidated()
{
    if (ServerPinger.AllServersValidated()) {
        auto servers = ServerPinger.GetPingedServers();

        FetchGameServer(FrontendApi, GlobalAccessToken, servers);
        ServerPinger.ClearPingedServers();
        GetWorld()->GetTimerManager().ClearTimer(MemberTimerHandle);
    }
}
