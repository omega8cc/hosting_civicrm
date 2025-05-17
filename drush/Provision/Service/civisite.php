<?php

/**
 * The CiviCRM service class.
 */
class Provision_Service_civisite extends Provision_Service {
  public $service = 'civisite';

  static function subscribe_civisite($context) {
    drush_log('CiviCRM: service subscribe_civisite');
    $context->is_oid('civisite');

    // Copied from provision/db/Provision/Service/db.php
    $context->setProperty('db_server', '@server_master');
    $context->is_oid('db_server');
    $context->service_subscribe('db', $context->db_server->name);

    // Required for civicrm-delete (to delete the vhost file).
    $context->service_subscribe('http', $context->web_server->name);

    // Drushrc needs this to find the drushrc.php file
    $context->setProperty('site_path');

    // Load the drushrc.
    // Since civisite is not a valid drush context, we need to do it manually.
    // This will load variables in $_SERVER.
    $config = new Provision_Config_Drushrc_civisite($context->name);
    $filename = $config->filename();

    if ($filename && file_exists($filename)) {
      drush_log(dt('CiviCRM: loading !file', array('!file' => $filename)));
      global $options;

      include($filename);

      // This is necessary for deleting the DB in delete.civicrm.provision.inc
      foreach (array('db_type', 'db_host', 'db_port', 'db_passwd', 'db_name', 'db_user', 'civi_content_dir', 'civi_content_url') as $x) {
        drush_set_option($x, $options[$x], 'site');
      }
    }
  }

  static function subscribe_civiplatform($context) {
    $context->is_oid('civiplatform');

    // Copied from provision/db/Provision/Service/db.php
    $context->setProperty('db_server', '@server_master');
    $context->is_oid('db_server');
    $context->service_subscribe('db', $context->db_server->name);
  }

}
