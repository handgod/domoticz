#include "stdafx.h"
#include "WebServerHelper.h"
#include "../main/SQLHelper.h"

namespace http {
	namespace server {

		typedef std::vector<CWebServer*>::iterator server_iterator;
		typedef std::vector<CProxyManager*>::iterator proxy_iterator;

		CWebServerHelper::CWebServerHelper()
		{
#ifdef NS_ENABLE_SSL
			secureServer_ = NULL;
#endif
			plainServer_ = NULL;
		}

		CWebServerHelper::~CWebServerHelper()
		{
#ifdef NS_ENABLE_SSL
			if (secureServer_) delete secureServer_;
#endif
			if (plainServer_) delete plainServer_;

			for (proxy_iterator it = proxymanagerCollection.begin(); it != proxymanagerCollection.end(); ++it) {
				delete (*it);
			}
		}

		bool CWebServerHelper::StartServers(std::string &listenaddress, const std::string &listenport, const std::string &secure_listenport, const std::string &serverpath, const std::string &secure_cert_file, const std::string &secure_cert_passphrase, const bool bIgnoreUsernamePassword)
		{
			bool bRet = false;

#ifdef NS_ENABLE_SSL
			SSL_library_init();
			serverCollection.resize(secure_listenport.empty() ? 1 : 2);
#else
			serverCollection.resize(1);
#endif
			plainServer_ = new CWebServer();
			serverCollection[0] = plainServer_;
			bRet |= plainServer_->StartServer(listenaddress, listenport, serverpath, bIgnoreUsernamePassword);
#ifdef NS_ENABLE_SSL
			if (!secure_listenport.empty()) {
				secureServer_ = new CWebServer();
				bRet |= secureServer_->StartServer(listenaddress, secure_listenport, serverpath, bIgnoreUsernamePassword, secure_cert_file, secure_cert_passphrase);
				serverCollection[1] = secureServer_;
			}
#endif

#if 1
			// RK: TODO
			//m_sql.UpdatePreferencesVar("MyDomoticzInstanceId", "");
			const int connections = GetNrMyDomoticzThreads();
			proxymanagerCollection.resize(connections);
			for (int i = 0; i < connections; i++) {
				proxymanagerCollection[i] = new CProxyManager(serverpath, plainServer_->m_pWebEm);
				proxymanagerCollection[i]->Start();
			}
			_log.Log(LOG_STATUS, "Proxymanager started.");	
#endif

			return bRet;
		}

		void CWebServerHelper::StopServers()
		{
			for (server_iterator it = serverCollection.begin(); it != serverCollection.end(); ++it) {
				(*it)->StopServer();
			}
			for (proxy_iterator it = proxymanagerCollection.begin(); it != proxymanagerCollection.end(); ++it) {
				(*it)->Stop();
			}
		}

		void CWebServerHelper::SetAuthenticationMethod(int amethod)
		{
			for (server_iterator it = serverCollection.begin(); it != serverCollection.end(); ++it) {
				(*it)->SetAuthenticationMethod(amethod);
			 }
		}

		void CWebServerHelper::SetWebTheme(const std::string &themename)
		{
			for (server_iterator it = serverCollection.begin(); it != serverCollection.end(); ++it) {
				(*it)->SetWebTheme(themename);
			 }
		}

		void CWebServerHelper::ClearUserPasswords()
		{
			for (server_iterator it = serverCollection.begin(); it != serverCollection.end(); ++it) {
				(*it)->ClearUserPasswords();
			 }
		}

		//JSon
		void CWebServerHelper::	GetJSonDevices(Json::Value &root, const std::string &rused, const std::string &rfilter, const std::string &order, const std::string &rowid, const std::string &planID, const std::string &floorID, const bool bDisplayHidden, const time_t LastUpdate, const bool bSkipUserCheck)
		{
			if (plainServer_) { // assert
				plainServer_->GetJSonDevices(root, rused, rfilter, order, rowid, planID, floorID, bDisplayHidden, LastUpdate, bSkipUserCheck);
			}
#ifdef NS_ENABLE_SSL
			else if (secureServer_) {
				secureServer_->GetJSonDevices(root, rused, rfilter, order, rowid, planID, floorID, bDisplayHidden, LastUpdate, bSkipUserCheck);
			}
#endif
		}

		void CWebServerHelper::ReloadCustomSwitchIcons()
		{
			for (server_iterator it = serverCollection.begin(); it != serverCollection.end(); ++it) {
				(*it)->ReloadCustomSwitchIcons();
			 }
		}

		int CWebServerHelper::GetNrMyDomoticzThreads()
		{
			std::string instanceid;
			int nrThreads = 3;
			m_sql.GetPreferencesVar("MyDomoticzInstanceId", instanceid);
			if (instanceid == "") {
				return 1;
			}
			m_sql.GetPreferencesVar("MyDomoticzNrThreads", nrThreads);
			return nrThreads;
		}
	}

}