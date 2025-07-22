<?php

/**
 * implementation of hook_post_provision_install
 */
function drush_provision_civicrm_post_provision_install($url = null) {
  if (! _provision_civicrm_is_site_context(TRUE)) {
    return;
  }

  drush_log(dt("CiviCRM: Starting installation..."));

  $db_user = drush_get_option('db_user', NULL, 'site');
  $db_passwd = drush_get_option('db_passwd', NULL, 'site');
  $db_name = drush_get_option('db_name', NULL, 'site');

  // Always setting the port helps support non-standard aegir configs,
  // but also workaround CRM-19407.
  //
  // FIXME: this is not if the hostname is using an IPv6 address
  // but 1- use a hostname instead, and 2- needs fixing in civicrm.drush.inc.
  $db_host = drush_get_option('db_host', NULL, 'site') . ':' . drush_get_option('db_port', 3306, 'site');

  // Inherit the language from Drupal
  // see _provision_civicrm_language_uf_to_civicrm() for details.
  $lang = '';

  $uf_language = d()->language;
  drush_log(dt("CiviCRM: Drupal language: %lang", array('%lang' => $uf_language)));

  if ($uf_language && $uf_language != 'en') {
    $lang = _provision_civicrm_language_uf_to_civicrm($uf_language);
    drush_log(dt("CiviCRM: CiviCRM language: %lang", array('%lang' => $lang)));
  }

  _provision_civicrm_install_civicrm($db_user, $db_passwd, $db_host, $db_name, $lang);
  drush_log(dt("CiviCRM: Installation complete!"), 'ok');
}

/**
 * Main function to initialize the CiviCRM database and settings file.
 * It used to do more things, but most of it was removed when we switched
 * to just calling a drush command, depending on the Drupal version.
 */
function _provision_civicrm_install_civicrm($dbuser, $dbpass, $dbhost, $dbname, $lang) {
  // As of D8, the database schema and civicrm.settings.php
  // are generated when the module is first installed.

  // Install CiviCRM using cv and then enable the module
  // @todo FIXME On Drupal10, test if we can use cv core:install as well (so that we can support the $lang)
  drush_log(dt('CiviCRM: Detected Drupal core version: %version', ['%version' => drush_drupal_major_version()]), 'ok');
  $protocol = (d()->ssl_enabled || d()->https_enabled) ? 'https' : 'http';
  $baseUrl = $protocol . '://' . d()->uri . '/';
  $command = 'cv core:install --cms-base-url=' . $baseUrl . ' --lang=' . escapeshellarg($lang) . ' && drush pm-enable civicrm -y';

  if (drush_drupal_major_version() >= 10) {

    // Find The App Root Directory
    $appRoot = findDrupalAppRoot(d()->root);

    // Define Local Drush Path
    $drushPath = 'vendor/drush/drush/drush.php';

    // Unlock Site-Local Drush
    provision_check_codebase_status();
    if (d()->isConsolePatched || d()->isCorePatched) {
      drush_log(dt('CRM/LOCAL/DRUSH/UNLOCK with provision_unlock_drush_local_vnd'), 'info');
      provision_unlock_drush_local_vnd();
    }

    // Check If The Local Drush Exists And Is Executable
    $drushAbsolutePath = $appRoot . '/' . $drushPath;
    drush_log(dt('CRM/LOCAL/DRUSH/CHECK Path @path', array('@path' => $drushAbsolutePath)), 'info');

    if (is_executable($drushAbsolutePath)) {
      $cmd_string = $drushPath . ' pm:install civicrm -y';
    }
    else {
      $cmd_string = 'drush10 pm:install civicrm -y';
    }

    // Make site_path writable
    provision_file()->chmod(d()->site_path, 0755);

    drush_log(dt("CiviCRM: running: %command", ['%command' => $cmd_string]), 'ok');

    $result = drush_shell_cd_and_exec($appRoot, $cmd_string);
    $output = drush_shell_exec_output();
    foreach ($output as $index => $line) {
      if (!$success) { // Log the entire error as a notice.
        $line_status = 'notice';
      }
      else { // Only log the last line as success.
        $line_status = $index+1 == count($output) ? 'success' : 'notice';
      }
      drush_log($line, $line_status);
    }

    // Lock Site-Local Drush if Unlocked
    provision_check_codebase_status();
    if (!d()->isConsolePatched || !d()->isCorePatched) {
      drush_log(dt('CRM/LOCAL/DRUSH/LOCK with provision_lock_drush_local_vnd'), 'info');
      provision_lock_drush_local_vnd();
    }
  }
  else {
    drush_log(dt("CiviCRM: running: %command", ['%command' => $command]), 'ok');
    provision_file()->chmod(d()->site_path, 0755);
    $output = system($command, $retval);
    drush_log(dt("CiviCRM: retval=%retval, output: %output", ['%retval' => $retval, '%output' => $output]), $retval == 0 ? 'ok' : 'error');
  }

  provision_file()->chmod(d()->site_path, 0555);

  // Fix file permissions so that www-data can write
  drush_log(dt("CiviCRM: running fix-drupal-site-ownership.sh and permissions..."), 'ok');
  drush_shell_exec("sudo --non-interactive /usr/local/bin/fix-drupal-site-ownership.sh --site-path=%s --script-user=%s --web-group=%s", d()->site_path, d()->server->script_user, d()->server->web_group);
  $output = implode('', drush_shell_exec_output());
  drush_log(dt("CiviCRM: fix-drupal-site-ownership.sh output: %output", ['%output' => $output]), 'ok');
  drush_shell_exec("sudo --non-interactive /usr/local/bin/fix-drupal-site-permissions.sh --site-path=%s", d()->site_path);
  $output = implode('', drush_shell_exec_output());
  drush_log(dt("CiviCRM: fix-drupal-site-permissions.sh output: %output", ['%output' => $output]), 'ok');
  drush_log(dt("CiviCRM: finished running fix-drupal-site-ownership.sh and permissions"), 'ok');
}

/**
 * Returns the CiviCRM language (ex: fr_FR) for a Drupal language (ex: fr).
 *
 * Handles a few exceptions (zh_CN, af_ZA, etc) and can be overridden with a
 * constant in your aegir global.inc of the form:
 * PROVISION_CIVICRM_LANGUAGE_MAPPING_XX, where XX is the short language code
 * from Drupal.
 */
function _provision_civicrm_language_uf_to_civicrm($uf_language) {
  if (defined('PROVISION_CIVICRM_LANGUAGE_MAPPING_' . $uf_language)) {
    return constant('PROVISION_CIVICRM_LANGUAGE_MAPPING_' . $uf_language);
  }

  // Taken from l10n/bin/copy-to-svn.sh
  $exceptions = array(
    'ar' => 'ar_EG', // Arabic/Egypt
    'af' => 'af_ZA', // Afrikaans/South Africa
    'be' => 'be_BY', // Belarus
    'ca' => 'ca_ES', // Catalan/Spain
    'cs' => 'cs_CZ', // Czech
    'da' => 'da_DK', // Danish
    'el' => 'el_GR', // Greek
    'et' => 'et_EE', // Estonian
    'he' => 'he_IL', // Hebrew/Israel
    'hi' => 'hi_IN', // Hindi
    'hy' => 'hy_AM', // Armenian
    'ja' => 'ja_JP', // Japanese/Japan
    'sv' => 'sv_SE', // Swedish
    'sl' => 'sl_SI', // Slovenian
    'sq' => 'sq_AL', // Albenian
    'sr' => 'sr_RS', // Serbian
    'vi' => 'vi_VN', // Vietnam
    'zh' => 'zh_CN', // Mandarin/China
  );

  if (isset($exceptions[$uf_language])) {
    return $exceptions[$uf_language];
  }

  return $uf_language . '_' . strtoupper($uf_language);
}
