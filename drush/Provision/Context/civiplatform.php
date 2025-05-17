<?php

/**
 * @file CiviCRM Provision named context platform class.
 */

/**
 * Class for the platform context.
 */
class Provision_Context_civiplatform extends Provision_Context {
  public $parent_key = 'server';

  static function option_documentation() {
    return [
      'root' => 'civiplatform: path to a CiviCRM installation',
      'server' => 'platform: drush backend server; default @server_master',
      'web_server' => 'platform: web server hosting the platform; default @server_master',
    ];
  }

  function init_civiplatform() {
    $this->setProperty('root');
  }

}
