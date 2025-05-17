<?php

/**
 * Check that we are trying to install a new site, and a new site only
 */
function drush_provision_civicrm_install_validate() {
  drush_log('CiviCRM: starting drush_provision_civicrm_install_validate', 'ok');

  if (!d()->uri) {
    return drush_set_error("PROVISION_URL_REQUIRED", dt("You need to specify a valid url to install a site"));
  }

/* TODO
  if (_provision_drupal_site_exists()) {
    return drush_set_error('PROVISION_SITE_INSTALLED');
  }
*/

  drush_log('CiviCRM: finished drush_provision_civicrm_install_validate', 'ok');
}

/**
 * Implements the provision-civicrm-install command.
 *
 * Mostly copies /usr/share/drush/commands/provision/platform/reset.login.provision.inc
 * but for uid = 2.
 */
function drush_provision_civicrm_install() {
  drush_log('CiviCRM: starting drush_provision_civicrm_install', 'ok');

  // d()->service_subscribe('platform', d()->civiplatform);
  d()->service_subscribe('server', '@server_master'); // FIXME hardcoded name?
  d()->service_subscribe('http', '@server_master'); // FIXME hardcoded name?
  d()->service_subscribe('db', d()->db_server->name);

  d()->service('db')->create_site_database();
  d()->service('http')->create_config('site');
  $creds = d()->service('db')->fetch_site_credentials();

  // This makes the assumption that if LetsEncrypt is enabled on the web server,
  // the site will very soon be https-enabled.
  $server = d()->civiplatform->web_server;
  $protocol = ($server->Certificate_service_type == 'LetsEncrypt' ? 'https' : 'http');

  // FIXME .. this relies on something something that might be removed
  // from provision/db/Provision/Service/db.php.
  //
  // However, since we cloak the db setings in civi-config.php, and that
  // is also the simplest way to do multi-platform, ideally we should
  // have a proper Aegir API to fetch the DB credentials.
  $_SERVER['db_type'] = drush_get_option('db_type', NULL, 'site');
  $_SERVER['db_host'] = drush_get_option('db_host', NULL, 'site');
  $_SERVER['db_port'] = drush_get_option('db_port', NULL, 'site');
  $_SERVER['db_passwd'] = drush_get_option('db_passwd', NULL, 'site');
  $_SERVER['db_name'] = drush_get_option('db_name', NULL, 'site');
  $_SERVER['db_user'] = drush_get_option('db_user', NULL, 'site');
  // @todo?
  // $_SERVER['civi_content_dir'] = d()->site_path . '/civi-content';
  // $_SERVER['civi_content_url'] = $protocol . '://' . d()->uri . '/sites/' . d()->uri . '/civi-content';

  // Then we add it in the civisite context
  drush_set_option('db_type', $_SERVER['db_type'], 'civisite');
  drush_set_option('db_host', $_SERVER['db_host'], 'civisite');
  drush_set_option('db_port', $_SERVER['db_port'], 'civisite');
  drush_set_option('db_passwd', $_SERVER['db_passwd'], 'civisite');
  drush_set_option('db_name', $_SERVER['db_name'], 'civisite');
  drush_set_option('db_user', $_SERVER['db_user'], 'civisite');
  // drush_set_option('civi_content_dir', $_SERVER['civi_content_dir'], 'civisite');
  // drush_set_option('civi_content_url', $_SERVER['civi_content_url'], 'civisite');

  // And now save the civisite context to disk (drushrc.php).
  $config = new Provision_Config_Drushrc_civisite(d()->name);
  $config->write();

  // Run the CiviCRM installer
  $admin_email = drush_get_option('client_email', 'fixme@example.org');

  // @todo --lang=fr_CA, admin email?
  $dsn = 'mysql://' . $_SERVER['db_name'] . ':' . $_SERVER['db_passwd'] . '@' . $_SERVER['db_host'] . ':' . $_SERVER['db_port'] . '/' . $_SERVER['db_name'];
  _provision_civicrm_run_cv_command('core:install --db=' . $dsn . ' --cms-base-url=https://' . d()->uri . ' -m extras.adminEmail=' . $admin_email);

  d()->service('http')->restart();

  drush_log('CiviCRM: finished drush_provision_civicrm_install', 'ok');
}
