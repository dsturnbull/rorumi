require 'rubygems'
require 'mechanize'
require 'json'

module LocationFinder
  class FindMyIphone
    def self.find(username, password)
      agent = Mechanize.new
      login_page = agent.get('https://auth.me.com/authenticate?service=findmyiphone&ssoNamespace=appleid')
      login_form = login_page.forms.first
      login_form.username = username
      login_form.password = password
      agent.submit(login_form, login_form.buttons.first)

      fmi_uri = 'https://p01-fmipweb.me.com/fmipservice/client'
      info = JSON.parse(agent.post("#{fmi_uri}/initClient").content)

      info['content'].each do |device|
        params = { :clientContext => :shouldLocate,
                   :selectedDevice => device['id'] }.to_json

        json = agent.post("#{fmi_uri}/refreshClient").content
        JSON.parse(json)['content']
      end
    end
  end
end
