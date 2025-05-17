<?php

/**
 * The CiviCRM platform service class.
 */
class Provision_Service_civiplatform extends Provision_Service {
  public $service = 'civiplatform';

  function init_civiplatform() {
    // FIXME: this isn't called, because we define the service as civiplatform=>NULL
    // in hook_provision_services().. but if we define a 'default' service, we get:
    // Fatal error: Call to a member function setContext() on null in /usr/share/drush/commands/provision/Provision/Context/server.php on line 119
    drush_log('CiviCRM: service init_civiplatform');
    $this->configs['civiplatform'] = array('Provision_Config_civiplatform');
  }

  function verify_civiplatform_cmd() {
    drush_log('CiviCRM: service verify_civiplatform_cmd');
    $this->create_config($this->context->type);
    $this->parse_configs();
  }

  static function subscribe_civiplatform($context) {
    drush_log('CiviCRM: service subscribe_civiplatform');
    $context->is_oid('civiplatform');

    // Copied from provision/http/Provision/Service/http.php
    // Not sure if necessary.
    $context->setProperty('web_server', $context->web_server->name);
    $context->is_oid('web_server');
    $context->service_subscribe('http', $context->web_server->name);
  }
}
